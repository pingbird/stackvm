import 'dart:io';
import 'dart:math';

import 'package:pedantic/pedantic.dart';
import 'package:yaml_edit/yaml_edit.dart';
import 'package:yaml/yaml.dart';
import 'package:crypto/crypto.dart' as crypto;
import 'package:csv/csv.dart' as csv;

class BenchmarkResult {
  BenchmarkResult({
    required this.totalTime,
    required this.outputHash,
  });

  final int totalTime;
  final String outputHash;
}

Future<Process> startLinuxProcess(String command, List<String> args, {String? workingDirectory}) {
  if (workingDirectory != null && !Directory(workingDirectory).existsSync()) {
    throw AssertionError('Working directory does not exist: $workingDirectory');
  }
  return Process.start(Platform.isLinux ? command : 'wsl', [
    if (!Platform.isLinux) command,
    ...args,
  ], workingDirectory: workingDirectory);
}

Future<BenchmarkResult?> runBenchmark({
  required String program,
  String? inputFile,
  int batch = 1,
  int width = 8
}) async {
  var tempDir = Directory('temp');
  if (tempDir.existsSync()) {
    await tempDir.delete(recursive: true);
  }
  tempDir.createSync();

  var proc = await startLinuxProcess('cmake-build-release/stackvm', [
    '-w', '$width',
    '-p', '$batch',
    '-d', 'temp',
    '-q',
    '-m', '0,2048',
    if (inputFile != null) ...[
      '-i', inputFile
    ],
    program,
  ]);

  unawaited(proc.stdout.drain());
  final stderrFinish = proc.stderr.listen(stderr.add).asFuture();
  await proc.stdin.flush();
  await proc.stdin.close();

  int? exitCode;

  await Future.any([
    Future.delayed(Duration(seconds: 60)),
    proc.exitCode.then((value) => exitCode = value),
  ]);

  if (exitCode == null) {
    proc.kill(ProcessSignal.sigkill);
    return null;
  } else if (exitCode != 0) {
    await stderrFinish;
    stderr.writeln('stackvm failed with exit code $exitCode');
    exit(1);
  }

  var outputHash = crypto.sha1.convert(await File('temp/output.txt').readAsBytes()).toString();
  var timeline = csv.CsvToListConverter().convert(await File('temp/timeline.txt').readAsString());

  late int batchStart;
  late int batchEnd;

  for (var i = 1; i < timeline.length; i++) {
    var row = timeline[i];
    var timestamp = row[0];
    var event = row[1];
    var label = row[2];
    if (event == 'start') {
      if (label == 'Batch') {
        batchStart = timestamp;
      }
    } else if (event == 'finish') {
      if (label == 'Batch') {
        batchEnd = timestamp;
      }
    }
  }

  return BenchmarkResult (
    totalTime: batchEnd - batchStart,
    outputHash: outputHash,
  );
}

void main(List<String> args) async {
  final benchmarkYamlFile = File('benchmark.yaml');

  var yamlText = await File('config.yaml').readAsString();
  var editor = YamlEditor(yamlText);
  var yamlData = loadYaml(yamlText);

  var benchmarkYamlText = benchmarkYamlFile.existsSync()
      ? await benchmarkYamlFile.readAsString() : '';
  var benchmarkYamlData = loadYaml(benchmarkYamlText) ?? <String, dynamic>{};

  benchmarkYamlData['benchmarks'] ??= <String, dynamic>{};

  var res = await startLinuxProcess('make', ['-j', '12'], workingDirectory: 'cmake-build-release');
  final stderrSub = res.stderr.listen(stderr.add);
  unawaited(res.stdout.drain());
  final exitCode = await res.exitCode;
  if (exitCode != 0) {
    await stderrSub.asFuture();
    stderr.writeln('Error: make exited with status code $exitCode');
    exit(1);
  }

  var benchmarks = yamlData['benchmarks'] as YamlList;
  for (var benchmark = 0; benchmark < benchmarks.length; benchmark++) {
    var benchmarkInfo = benchmarks[benchmark];
    var benchmarkName = benchmarkInfo['name'] as String;
    var program = benchmarkInfo['src'];
    var width = 8;

    if (benchmarkInfo['width'] != null) {
      width = benchmarkInfo['width'];
    }

    final inputFile = benchmarkInfo['input'] as String?;

    final benchmarkRunData =
      benchmarkYamlData?['benchmarks']?[benchmarkName] ??= <String, dynamic>{};

    var batch = benchmarkYamlData?['benchmarks']?[benchmarkName]?['batch'] as int?;
    var baseline = benchmarkYamlData?['benchmarks']?[benchmarkName]?['baseline'] as int?;
    var outputHash = benchmarkInfo['output'];

    if (batch == null) {
      stderr.writeln('Calibrating $benchmarkName...');
      for (var i = 0;;i++) {
        final profileBatch = pow(10, i).toInt();
        var res = await runBenchmark(
          program: program,
          inputFile: inputFile,
          batch: profileBatch,
          width: width,
        );

        if (res == null) {
          stderr.writeln("Benchmark '$benchmarkName' timed out.");
          exit(1);
        }

        stderr.writeln('${profileBatch}x - ${res.totalTime ~/ profileBatch}ns');

        const minNanoseconds = 10000000; // 10ms
        const idealNanoseconds = 5000000000; // 5s

        if (res.totalTime > minNanoseconds) {
          batch = (idealNanoseconds / (res.totalTime / profileBatch)).ceil();
          benchmarkRunData['batch'] = batch;
          stderr.writeln('new batch: $batch');
          break;
        }
      }
    }

    var res = await runBenchmark(
      program: program,
      inputFile: inputFile,
      batch: batch,
      width: width,
    );

    if (res == null) {
      stderr.writeln("Benchmark '$benchmarkName' timed out.");
      exit(1);
    }

    if (baseline == null) {
      benchmarkRunData['baseline'] = res.totalTime;
    } else {
      var percentage = (((res.totalTime - baseline) / baseline) * 1000).round() / 10;
      print('[$benchmarkName] ${percentage < 0 ? '' : '+'}${percentage.toStringAsFixed(1)}%');
    }

    if (outputHash == null) {
      editor.update(['benchmarks', benchmark, 'output'], res.outputHash);
    } else {
      if (outputHash != res.outputHash) {
        stderr.writeln('[$benchmarkName] Output mismatch ${res.outputHash} vs $outputHash');
        exit(1);
      }
    }
  }

  if (editor.edits.isNotEmpty) {
    await File('config.yaml').writeAsString(editor.toString());
  }

  final newBenchmarkYamlText =
    (YamlEditor('')..update([], benchmarkYamlData)).toString();
  if (newBenchmarkYamlText != benchmarkYamlText) {
    await File('benchmark.yaml').writeAsString(newBenchmarkYamlText);
  }

  exit(0);
}
