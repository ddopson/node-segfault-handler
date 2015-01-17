var spawn = require('child_process').spawn;
var seg   = spawn('node', ['example.js']);
var test  = require('tap').test;

test('simple seg fault', function (t) {
   t.plan(1);
   var errdataevents = 0;   
   seg.stdout.on('data', function (data) {
      console.log('stdout: ' + data);
   });

   seg.stderr.on('data', function (data) {
      console.log('stderr: ' + data);
      errdataevents++;
   });

   seg.on('close', function (code) {
         t.ok(code > 0, 'Error code is greater than 0');
         console.log('child process exited with code ' + code);
   });
});
