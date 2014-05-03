
var audioElements, playingTrack;
var stoppedPosition = { 'track': 1, 'time': 0 };

function createAudioElements(from, to) {
  if (audioElements)
    return;
  audioElements = [];
  for (var i = from; i <= to; i++) {
    var el = document.createElement('audio');
    el.setAttribute('src', 'game/kichiku/' + ('0' + i).substr(-2) + '.ogg');
    el.setAttribute('preload', 'none');
    document.body.appendChild(el);
    audioElements[i] = el;
  }
}

function cd_play(track) {
  createAudioElements(2, 29);
  if (audioElements[track]) {
    cd_stop();
    if (audioElements[track].readyState == 0)
      audioElements[track].load();
    else
      audioElements[track].currentTime = 0;
    audioElements[track].play();
    playingTrack = track;
  }
}

function cd_stop() {
  if (playingTrack && audioElements[playingTrack]) {
    audioElements[playingTrack].pause();
    stoppedPosition = cd_getposition();
    playingTrack = null;
  }
}

function cd_getposition() {
  if (!playingTrack || !audioElements[playingTrack])
    return stoppedPosition;

  time = Math.round(audioElements[playingTrack].currentTime * 75);
  return playingTrack | time << 8;
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
  if (data.command == 'cd_play') {
    cd_play(data.track);
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
