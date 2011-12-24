try {
  // as of node 0.6.x, node-waf seems to build to a different directory. grr.
  var native = require(__dirname + '/../build/Release/segvhandler_native');
} catch(e) {
  var native = require(__dirname + '/../build/default/segvhandler_native');
}

module.exports = native;
