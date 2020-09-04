import 'dart:io';
import 'dart:typed_data';

import 'package:pedantic/pedantic.dart';
import 'package:yaml_edit/yaml_edit.dart';
import 'package:yaml/yaml.dart';
import 'package:crypto/crypto.dart' as crypto;
import 'package:csv/csv.dart' as csv;

class BenchmarkResult {
  int time;
  int inst;
  String outputHash;
}

Future<BenchmarkResult> runBenchmark({
  String program,
  Uint8List input,
  int batch = 1,
  int width = 8
}) async {
  print('running $program');
  var tempDir = Directory('temp');
  if (tempDir.existsSync()) {
    await tempDir.delete(recursive: true);
  }

  var cmd = 'cmake-build-release/stackvm';
  var proc = await Process.start(Platform.isLinux ? cmd : 'wsl', [
    if (!Platform.isLinux) cmd,
    '-w', '$width',
    '-p', '$batch',
    '-d', 'temp',
    program,
  ]);

  proc.stdin.add(input);
  unawaited(proc.stdin.flush().then((_) {
    proc.stdin.close();
  }));

  int exitCode;

  await Future.any([
    Future.delayed(Duration(seconds: 200)),
    proc.exitCode.then((value) => exitCode = value),
  ]);

  print('exit: $exitCode');

  if (exitCode == null) {
    proc.kill(ProcessSignal.sigkill);
    return null;
  } else if (exitCode != 0) {
    await stderr.addStream(proc.stderr);
    stderr.writeln('stackvm failed with exit code $exitCode');
    exit(1);
  }

  var outputHash = crypto.sha1.convert(await File('temp/output.txt').readAsBytes()).toString();
  var timeline = csv.CsvToListConverter().convert(await File('temp/timeline.txt').readAsString());

  int batchStart;
  int batchEnd;

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

  /*
  var res = await Process.run(
    'wsl', [
      'clang',
      '-O2',
      'temp/llvm_ir_unopt.ll',
      'cmake-build-release/libstackvm-runtime.a'
      '-o', 'temp/program'
    ],
    stdoutEncoding: null,
    stderrEncoding: null,
  );

  if (res.exitCode != 0) {
    stderr.add(res.stderr);
  }

  res = await Process.run(
    '/home/pix/bin/perf', [
      'clang',
      '-O2',
      'temp/llvm_ir_unopt.ll',
      'cmake-build-release/libstackvm-runtime.a',
      '-o', 'temp/program'
    ],
    stdoutEncoding: null,
    stderrEncoding: null,
  );

  if (res.exitCode != 0) {
    stderr.add(res.stderr);
  }*/

  return BenchmarkResult()
    ..time = (batchEnd - batchStart) ~/ batch
    ..outputHash = outputHash;
}

void main(List<String> args) async {
  var yamlText = await File('config.yaml').readAsString();
  var editor = YamlEditor(yamlText);
  var yamlData = loadYaml(yamlText);

  /*
  var makeResult = await Process.run(
    'wsl', [
      'make',
      '-j12',
    ],
    workingDirectory: 'cmake-build-release',
    stdoutEncoding: null,
    stderrEncoding: null,
  );

  if (makeResult.exitCode != 0) {
    stderr.add(makeResult.stderr);
    exit(1);
  }*/

  var benchmarks = yamlData['benchmarks'] as YamlList;
  for (var i = 0; i < benchmarks.length; i++) {
    var benchmarkInfo = benchmarks[i];
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

    int batch = benchmarkInfo['batch'];

    if (batch == null) {
      var res = await runBenchmark(
        program: program,
        input: input,
        batch: 1,
        width: width,
      );

      if (res == null) {
        stderr.writeln("Benchmark '${name}' timed out.");
        exit(1);
      }

      batch = (1000000000 / (res.time - 7000)).ceil();

      editor.update(['benchmarks', i, 'batch'], batch);
    }

    var res = await runBenchmark(
      program: program,
      input: input,
      batch: batch,
      width: width,
    );

    if (res == null) {
      stderr.writeln("Benchmark '${name}' timed out.");
      exit(1);
    }

    var baseline = benchmarkInfo['baseline'];
    var outputHash = benchmarkInfo['output'];

    if (baseline == null) {
      editor.update(['benchmarks', i, 'baseline'], res.time);
    } else {
      var percentage = ((res.time / baseline) * 100).round() - 100;
      print('[$name] ${percentage < 0 ? '' : '+'}${percentage}%');
    }

    if (outputHash == null) {
      editor.update(['benchmarks', i, 'output'], res.outputHash);
    } else {
      if (outputHash != res.outputHash) {
        stderr.writeln('Output mismatch ${res.outputHash} vs ${outputHash}');
        exit(1);
      }
    }
  }

  if (editor.edits.isNotEmpty) {
    await File('config.yaml').writeAsString(editor.toString());
  }
}
