import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:pedantic/pedantic.dart';
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
  print('$command ${jsonEncode(args)}');
  if (workingDirectory != null && !Directory(workingDirectory).existsSync()) {
    throw AssertionError('Working directory does not exist: $workingDirectory');
  }
  return Process.start(Platform.isLinux ? command : 'wsl', [
    if (!Platform.isLinux) command,
    ...args,
  ], workingDirectory: workingDirectory);
}

extension ListIntStreamExtensions on Stream<List<int>> {
  Future<Uint8List> toBytes() {
    final completer = Completer<Uint8List>();
    final sink = ByteConversionSink.withCallback(
      (bytes) => completer.complete(Uint8List.fromList(bytes)),
    );
    listen(
      sink.add,
      onError: completer.completeError,
      onDone: sink.close,
      cancelOnError: true,
    );
    return completer.future;
  }
}

Future<BenchmarkResult?> runBenchmark({
  required String name,
  required String program,
  String? inputFile,
  List<int>? input,
  int batch = 1,
  int? width,
  String mode = 'release',
  String? memory = '0,2048',
  bool profiled = true,
}) async {
  assert(inputFile == null || input == null, 'Only one of inputFile or inputString expected');
  var tempDir = Directory('temp/$name');
  if (tempDir.existsSync()) {
    await tempDir.delete(recursive: true);
  }
  tempDir.createSync(recursive: true);

  final start = DateTime.now();

  var proc = await startLinuxProcess('cmake-build-$mode/stackvm', [
    if (width != null) ...[
      '-w', '$width',
    ],
    if (profiled) ...[
      '-p', '$batch',
      '-d', tempDir.path,
      '-q',
    ],
    if (memory != null) ...[
      '-m', memory,
    ],
    if (inputFile != null) ...[
      '-i', inputFile,
    ],
    program,
  ]);

  late Future<Uint8List> stdout;

  if (profiled) {
    unawaited(proc.stdout.drain());
  } else {
    stdout = proc.stdout.toBytes();
  }

  final stderrFinish = proc.stderr.listen(stderr.add).asFuture();
  if (input != null) {
    proc.stdin.add(input);
  }
  unawaited(proc.stdin.flush().then((_) => proc.stdin.close()));

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
    stderr.writeln('stackvm benchmark "$name" failed with exit code $exitCode');
    exit(1);
  }

  if (!profiled) {
    return BenchmarkResult(
      totalTime: DateTime.now().difference(start).inMicroseconds * 1000,
      outputHash: crypto.sha1.convert(await stdout).toString(),
    );
  }

  var outputHash = crypto.sha1.convert(await File('${tempDir.path}/output.txt').readAsBytes()).toString();
  var timeline = csv.CsvToListConverter().convert(await File('${tempDir.path}/timeline.txt').readAsString());

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

Future<void> runMake(Iterable<String> modes) async {
  for (final mode in modes) {
    var res = await startLinuxProcess('make', ['-j', '12'], workingDirectory: 'cmake-build-$mode');
    final stderrSub = res.stderr.listen(stderr.add);
    unawaited(res.stdout.drain());
    final exitCode = await res.exitCode;
    if (exitCode != 0) {
      await stderrSub.asFuture();
      stderr.writeln('Error: make exited with status code $exitCode');
      exit(1);
    }
  }
}