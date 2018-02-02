'use strict';

var fs = require('fs');

var bootDir = `${__dirname}/../../ForBoot/`;
if (fs.existsSync('/boot')) {
  bootDir = '/boot';
  console.log('Running on raspberry pi');
}

var obtains = [
  './src/gridControl.js',
  'µ/color.js',
  'µ/utilities.js',
  `${bootDir}/gridConfig/config.js`,
  'child_process',
];

obtain(obtains, ({ Grid }, { rainbow }, { zeroPad }, { config }, { exec })=> {
  console.log(config);
  exports.app = {};

  let grid = new Grid();

  var aud = `${bootDir}/gridConfig/notes`;

  var clips = [];
  fs.readdir(aud, (err, files)=> {
    if (err) console.error(err);
    console.log('here');
    files.forEach((file, i, arr)=> {
      clips[i] = new Audio(`${aud}/${file}`);
    });
  });

  var cells = [];

  var reqTimer = null;
  var pulseTimer = null;
  var pulseCount = 0;
  var colRead = 0;

  exports.app.start = ()=> {
    exec('amixer set PCM -- 100%');
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

      setInterval(grid.setNextActive, config.tempo);
    };

    grid.onNextActive = (data, which)=> {
      if (data && data.length) {
        data.forEach(function (value, ind, arr) {
          if (!value) {
            console.log('strike!');
            if (clips[ind]) {
              clips[ind].currentTime = 0;
              clips[ind].play();
            }
          }
        });
      }

    };

    grid.onCellChange = (col, row, val)=> {
      console.log(`${col}:${row} was changed`);
      cells[col][row].classList.toggle('occ', val);
    };

    console.log('started');
  };

  //provide(exports);
});
