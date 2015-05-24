var gamedir = 'game/kichiku';
var bgmdir = null;

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
  audioElement.setAttribute('src', bgmdir + 'track' + track + '.wav');
  audioElement.setAttribute('controls', true);
  document.getElementById('contents').appendChild(audioElement);
  audioElement.trackno = track;
  audioElement.load();
  audioElement.loop = (loop != 0);
  audioElement.play();
}

function cd_stop() {
  if (audioElement) {
    audioElement.pause();
    audioElement.parentNode.removeChild(audioElement);
    audioElement = null;
  }
}

function cd_getposition() {
  if (!audioElement || audioElement.ended)
    return 0;

  time = Math.round(audioElement.currentTime * 75);
  return audioElement.trackno | time << 8;
}

function reply(data, value) {
  result = { 'result': value,
             'naclmsg_id': data['naclmsg_id'] };
  // console.log(result);
  xsystem35.postMessage({'naclmsg':result});
}

// This function is called when a message is received from the NaCl module.
function handleMessage(message) {
  var data = message.data;
  if (data.command == 'set_window_size') {
    setWindowSize(data.width, data.height);
  } else if (data.command == 'cd_play') {
    cd_play(data.track, data.loop);
  } else if (data.command == 'cd_stop') {
    cd_stop();
  } else if (data.command == 'cd_getposition') {
    reply(data, cd_getposition());
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

  webkitRequestFileSystem(PERSISTENT, 0, function(fs) {
    bgmdir = fs.root.toURL();
  });
})();
