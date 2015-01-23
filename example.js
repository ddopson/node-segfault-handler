#!/usr/bin/env node
var SegfaultHandler = require('./'); // you'd use require('segfault-handler')

SegfaultHandler.registerHandler();

SegfaultHandler.causeSegfault(); // simulates a buggy native module that dereferences NULL
