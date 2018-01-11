'use strict';

obtain(['./src/gridControl.js', 'µ/color.js'], ({ Grid }, { rainbow })=> {
  exports.app = {};

  let grid = new Grid();

  var clips = [];
  // clips[4] = new Audio('audio/note-e.wav');
  clips[3] = new Audio('audio/note-d.wav');
  clips[2] = new Audio('audio/note-c.wav');
  clips[1] = new Audio('audio/note-b.wav');
  clips[0] = new Audio('audio/note-a.wav');

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
      for (let i = 1; i < 17; i++) {
        setTimeout(()=> {
          var col = rainbow(i, 4).scale(.25);
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
          //cells[which][ind].clip.play();
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
