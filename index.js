var native = null;

if (native == null) {
  try {
    native = require('bindings')('segfault-handler-native');
  } catch (e) {
    //console.log("Node v0.10.X 'bindings' load mechanism failed with: " + e.stack);
  }
}

if (native == null) {
  try {
    // as of node 0.6.x, node-waf seems to build to a different directory. grr.
    native = require(__dirname + '/build/Release/segfault-handler-native');
  } catch(e) {
    //console.log("Node v0.6.x 'directory' load mechanism failed with: " + e.stack);
  }
}

if (native == null) {
  try {
    native = require(__dirname + '/build/default/segfault-handler-native');
  } catch(e) {
    //console.log("Node v0.4.x 'directory' load mechanism failed with: " + e.stack);
  }
}

if (native == null) {
  throw new Error("NodeSegfaultHandler: Failed to load native module");
}

module.exports = native;
