'use strict';

var bootDir = '../ForBoot';

var obtains = [
  './src/gridControl.js',
  'µ/color.js',
  'µ/utilities.js',
  `${bootDir}/gridConfig/config.js`,
];

obtain(obtains, ({ Grid }, { rainbow }, { zeroPad }, { config })=> {
  console.log(config);
  exports.app = {};

  let grid = new Grid();

  var aud = 'audio';
  aud = `${bootDir}/gridConfig/notes`;

  var clips = [];
  // clips[4] = new Audio('audio/note-e.wav');
  for (let i = 0; i < 14; i++) {
    console.log(`${aud}/note-${zeroPad((i + 1), 2)}.${config.fileType}`);
    clips[i] = new Audio(`${aud}/note-${zeroPad((i + 1), 2)}.${config.fileType}`);
  }

  var cells = [];

  var reqTimer = null;
  var pulseTimer = null;
  var pulseCount = 0;
  var colRead = 0;

  exports.app.start = ()=> {
    grid.setup();

    grid.onUpdateCount = ()=> {
      console.log(`Updated column count: ${grid.states.length} columns`);

      for (var i = 0; i < grid.states.length; i++) {
        var col = µ('+div', µ('#grid'));
        col.className = 'col';
        col.style.width = 'calc(90vw / ' + grid.states.length + ' )';
        cells[i] = [];
        for (var j = 0; j < clips.length; j++) {
          cells[i][j] = µ('+div', col);
          cells[i][j].clip = clips[j].cloneNode(true);

        }
      }
    };

    grid.onReady = ()=> {
      for (let i = 1; i < clips.length + 1; i++) {
        setTimeout(()=> {
          var col = rainbow(i, clips.length).scale(.25);
          grid.setRowColor(i, col[0], col[1], col[2]);
        }, 100 * i);
      }

      setInterval(grid.setNextActive, 200);
    };

    grid.onNextActive = (data, which)=> {
      data.forEach(function (value, ind, arr) {
        if (!value) {
          console.log('strike!');
          cells[which][ind].clip.currentTime = 0;
          cells[which][ind].clip.play();
        }
      });
    };

    grid.onCellChange = (col, row, val)=> {
      //console.log(`${col}:${row} was changed`);
      cells[col][row].classList.toggle('occ', val);
    };

    console.log('started');
  };

  //provide(exports);
});
