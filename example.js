#!/usr/bin/env node
var segvhandler = require('./lib/segvhandler')

segvhandler.registerHandler();

console.log("About to cause a Segfault");
segvhandler.causeSegfault();
