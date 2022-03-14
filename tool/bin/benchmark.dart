import 'dart:io';
import 'dart:math';

import 'package:stackvm_tool/tool.dart';
import 'package:yaml/yaml.dart';
import 'package:yaml_edit/yaml_edit.dart';

void main(List<String> args) async {
  final benchmarkYamlFile = File('benchmark.yaml');

  var yamlText = await File('config.yaml').readAsString();
  var editor = YamlEditor(yamlText);
  var yamlData = loadYaml(yamlText);

  var benchmarkYamlText = benchmarkYamlFile.existsSync()
      ? await benchmarkYamlFile.readAsString() : '';
  var benchmarkYamlData = loadYaml(benchmarkYamlText) ?? <String, dynamic>{};

  benchmarkYamlData['benchmarks'] ??= <String, dynamic>{};

  await runMake(['release']);

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
          name: benchmarkName,
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
      name: benchmarkName,
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