#!/usr/bin/env node
var segvhandler = require('./')

segvhandler.registerHandler();

console.log("About to cause a Segfault");
try {
  segvhandler.causeSegfault();
}catch (e) {
  console.log("Bye")
}
