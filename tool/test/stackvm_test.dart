import 'dart:io';

import 'package:stackvm_tool/tool.dart';
import 'package:test/test.dart';
import 'package:yaml/yaml.dart';

void main() async {
  final modes = ['debug', 'release', 'product'];
  Directory.current = Directory.current.parent;
  var yamlText = await File('config.yaml').readAsString();
  final yamlData = loadYaml(yamlText);
  await runMake(modes);

  for (final mode in modes) {
    test('$mode', () async {
      Future<void> runTest(dynamic benchmarkInfo) async {
        var res = await runBenchmark(
          name: benchmarkInfo['name'],
          program: benchmarkInfo['src'],
          inputFile: benchmarkInfo['input'],
          width: benchmarkInfo['width'],
        );
        expect(res, isNotNull, reason: 'Benchmark timed out');
      }
      await Future.wait((yamlData['benchmarks'] as YamlList).map(runTest));
    });
  }
}