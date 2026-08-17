var Module = {
  print: function(t) { self.__out.push(t); },
  printErr: function(t) { self.__out.push(t); },
  onRuntimeInitialized: function() { self.__ready = true; },
};
self.__out = [];
self.__ready = false;

importScripts("shish.js");

function waitForReady() {
  return new Promise(function(resolve) {
    (function poll() {
      if (self.__ready) resolve();
      else setTimeout(poll, 20);
    })();
  });
}

self.onmessage = function(ev) {
  var msg = ev.data;
  waitForReady().then(function() {
    try {
      FS.mkdir("/tests");
    } catch (e) {}
    msg.files.forEach(function(f) {
      FS.writeFile("/tests/" + f.name, f.text);
    });

    self.__out = [];
    var exitCode = null;
    try {
      var ret = Module.callMain(["tests/" + msg.name]);
      exitCode = typeof ret === "number" ? ret : 0;
    } catch (e) {
      if (e && e.name === "ExitStatus") {
        exitCode = e.status;
      } else {
        self.__out.push("[js exception] " + (e && e.message ? e.message : String(e)));
        exitCode = -1;
      }
    }
    postMessage({ name: msg.name, exitCode: exitCode, text: self.__out.join("\n") });
  });
};
