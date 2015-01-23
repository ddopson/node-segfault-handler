#!/usr/bin/env iojs
var SegfaultHandler = require('./'); // you'd use require('segfault-handler')

SegfaultHandler.registerHandler();

SegfaultHandler.causeSegfault(); // simulates a buggy native module that dereferences NULL
