import 'dart:io';
import 'dart:math';
import 'dart:typed_data';

import 'package:pedantic/pedantic.dart';
import 'package:yaml_edit/yaml_edit.dart';
import 'package:yaml/yaml.dart';
import 'package:crypto/crypto.dart' as crypto;
import 'package:csv/csv.dart' as csv;

class BenchmarkResult {
  BenchmarkResult({
    required this.time,
    required this.totalTime,
    required this.outputHash,
  });

  final int time;
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
  List<int>? input,
  int batch = 1,
  int width = 8
}) async {
  var tempDir = Directory('temp');
  if (tempDir.existsSync()) {
    await tempDir.delete(recursive: true);
  }

  var proc = await startLinuxProcess('cmake-build-release/stackvm', [
    '-w', '$width',
    '-p', '$batch',
    '-d', 'temp',
    '-q',
    '-m', '0,2048',
    program,
  ]);

  unawaited(proc.stdout.drain());
  final stderrFinish = proc.stderr.listen(stderr.add).asFuture();
  if (input != null) proc.stdin.add(input);
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
    time: (batchEnd - batchStart) ~/ batch,
    totalTime: batchEnd - batchStart,
    outputHash: outputHash,
  );
}

void main(List<String> args) async {
  var yamlText = await File('config.yaml').readAsString();
  var editor = YamlEditor(yamlText);
  var yamlData = loadYaml(yamlText);

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
    var name = benchmarkInfo['name'];
    var program = benchmarkInfo['src'];
    var input = Uint8List(0);
    var width = 8;

    if (benchmarkInfo['width'] != null) {
      width = benchmarkInfo['width'];
    }

    if (benchmarkInfo['input'] != null) {
      input = await File(benchmarkInfo['input']).readAsBytes();
    }

    int? batch = benchmarkInfo['batch'];

    if (batch == null) {
      stderr.writeln('Calibrating $name...');
      for (var i = 0;;i++) {
        final profileBatch = pow(10, i).toInt();
        var res = await runBenchmark(
          program: program,
          input: input,
          batch: profileBatch,
          width: width,
        );

        if (res == null) {
          stderr.writeln("Benchmark '$name' timed out.");
          exit(1);
        }

        stderr.writeln('${profileBatch}x - ${res.time}ns');

        const minNanoseconds = 10000000; // 10ms
        const idealNanoseconds = 5000000000; // 5s

        if (res.totalTime > minNanoseconds) {
          batch = (idealNanoseconds / (res.totalTime / profileBatch)).ceil();
          editor.update(['benchmarks', benchmark, 'batch'], batch);
          stderr.writeln('new batch: $batch');
          break;
        }
      }
    }

    var res = await runBenchmark(
      program: program,
      input: input,
      batch: batch,
      width: width,
    );

    if (res == null) {
      stderr.writeln("Benchmark '$name' timed out.");
      exit(1);
    }

    var baseline = benchmarkInfo['baseline'];
    var outputHash = benchmarkInfo['output'];

    if (baseline == null) {
      editor.update(['benchmarks', benchmark, 'baseline'], res.time);
    } else {
      var percentage = ((res.time / baseline) * 100).round() - 100;
      print('[$name] ${percentage < 0 ? '' : '+'}$percentage%');
    }

    if (outputHash == null) {
      editor.update(['benchmarks', benchmark, 'output'], res.outputHash);
    } else {
      if (outputHash != res.outputHash) {
        stderr.writeln('[$name] Output mismatch ${res.outputHash} vs $outputHash');
        exit(1);
      }
    }
  }

  if (editor.edits.isNotEmpty) {
    await File('config.yaml').writeAsString(editor.toString());
  }

  exit(0);
}
