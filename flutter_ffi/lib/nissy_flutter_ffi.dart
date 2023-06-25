import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';
import 'dart:typed_data';
import 'package:flutter/services.dart';

import 'package:ffi/ffi.dart';
import 'nissy_flutter_ffi_bindings_generated.dart';

void nissy_init(ByteData tables) {
  // Copy tables to C ffi heap, pass to actual nissy_init()
  final tablesList = tables.buffer.asUint8List();
  final n = tablesList.lengthInBytes;
  final tablesHeap = calloc<Char>(n);
  for (var i = 0; i < n; i++) {
    tablesHeap[i] = tablesList[i];
  }
  _bindings.nissy_init(tablesHeap);
}

String ptrCharToString(Pointer<Char> ptr) => ptr.cast<Utf8>().toDartString();
Pointer<Char> stringToPtrChar(String str) {
  final n = str.length;
  final strNative = str.toNativeUtf8().cast<Char>();
  final ptr = calloc<Char>(n+10);
  for (var i = 0; i < n; i++) {
    ptr[i] = strNative[i];
  }
  ptr[n] = 0;
  return ptr;
}

String nissy_test() {
  final ptr = calloc<Char>(50);
  _bindings.nissy_test(ptr);
  final str = ptrCharToString(ptr);
  calloc.free(ptr);
  return str;
}

List<String> nissy_eos_in(String scr, int n) {
  var ret = List<String>.empty(growable: true);

  final scramble = stringToPtrChar(scr);
  final eofb = stringToPtrChar('eofb');
  final uf = stringToPtrChar('uf');
  final normal = stringToPtrChar('normal');

  print('Doing thing...');

  var bufferPtr = calloc<Char>(5000); // hope it is enough
  final nsols = _bindings.nissy_solve(eofb, uf, n, normal, scramble, bufferPtr);
  ret.add(ptrCharToString(bufferPtr));

  print('Ret 0: ' + ret[0]);

  calloc.free(bufferPtr);
  calloc.free(normal);
  calloc.free(uf);
  calloc.free(eofb);
  calloc.free(scramble);

  return ret;
}

const String _libName = 'nissy_flutter_ffi';

/// The dynamic library in which the symbols for [NissyFlutterFfiBindings] can be found.
final DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    return DynamicLibrary.open('$_libName.framework/$_libName');
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// The bindings to the native functions in [_dylib].
final NissyFlutterFfiBindings _bindings = NissyFlutterFfiBindings(_dylib);
