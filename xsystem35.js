var gamedir = 'game/kichiku';

function setWindowSize(width, height) {
  var module = document.getElementById('nacl_module');
  module.setAttribute('width', width);
  module.setAttribute('height', height);
}

var audioElement;

function cd_play(track, loop) {
  if (audioElement)
    cd_stop();

  audioElement = document.createElement('audio');
  audioElement.setAttribute('src', gamedir + '/' + ('0' + track).substr(-2) + '.ogg');
  audioElement.setAttribute('controls', true);
  document.body.appendChild(audioElement);
  audioElement.trackno = track;
  audioElement.load();
  audioElement.loop = (loop != 0);
  audioElement.play();
}

function cd_stop() {
  if (audioElement) {
    audioElement.pause();
    document.body.removeChild(audioElement);
    audioElement = null;
  }
}

function cd_getposition() {
  if (!audioElement || audioElement.ended)
    return 0;

  time = Math.round(audioElement.currentTime * 75);
  return audioElement.trackno | time << 8;
}

// This function is called by common.js when the NaCl module is
// loaded.
function moduleDidLoad() {
  // Once we load, hide the plugin. In this example, we don't display anything
  // in the plugin, so it is fine to hide it.
  // common.hideModule();

  // After the NaCl module has loaded, common.naclModule is a reference to the
  // NaCl module's <embed> element.
  //
  // postMessage sends a message to it.
  // common.naclModule.postMessage('hello');
}

// This function is called by common.js when a message is received from the
// NaCl module.
function handleMessage(message) {
  var data = message.data;
  if (data.command == 'set_window_size') {
    setWindowSize(data.width, data.height);
  } else if (data.command == 'cd_play') {
    cd_play(data.track, data.loop);
  } else if (data.command == 'cd_stop') {
    cd_stop();
  } else if (data.command == 'cd_getposition') {
    result = { 'result': cd_getposition(),
               'naclmsg_id': data['naclmsg_id'] };
    // console.log(result);
    common.naclModule.postMessage(result);
  } else if (typeof data === 'string') {
    console.log(data);  // debug message
  } else {
    console.log('unknown message');
    console.log(message);
  }
}

(function() {
  var searchVars = {};
  if (window.location.search.length > 1) {
    var pairs = window.location.search.substr(1).split('&');
    for (var key_ix = 0; key_ix < pairs.length; key_ix++) {
      var keyValue = pairs[key_ix].split('=');
      searchVars[unescape(keyValue[0])] =
        keyValue.length > 1 ? unescape(keyValue[1]) : '';
    }
  }
  if (searchVars.gamedir)
    gamedir = searchVars.gamedir;
  document.body.setAttribute('data-attrs', 'gamedir=' + gamedir);
})();
