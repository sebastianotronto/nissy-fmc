import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'nissy_flutter_ffi_bindings_generated.dart';

String ptrCharToString(Pointer<Char> ptr) => ptr.cast<Utf8>().toDartString();

String nissy_test() {
  final ptr = calloc<Char>(50);
  _bindings.nissy_test(ptr);
  final str = ptrCharToString(ptr);
  calloc.free(ptr);
  return str;
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
