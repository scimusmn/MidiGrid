
obtain(['µ/serial.js'], (ser)=> {

  exports.Grid = function () {
    var _this = this;
    var serial = new ser.Serial(Buffer.from([128]));

    var BROADCAST = 127;
    var COMMAND = 128;
    var SEND_COUNT = 64;
    var GET_STATES = 16;
    var SET_COLOR = 32;
    var SET_DEFAULT_COLOR = 96;
    var SET_COLUMN_ACTIVE = 96;
    var RESET_COLUMN = 32;
    var END_FLAG = 0;
    var COMPLETE = 1;
    var SET_ACTIVE = 4;
    var RESET_COLOR = 2;
    var ROW_COLOR = 48;

    _this.states = [];
    numBoards = 0;
    var countRequested = true;
    var writingActive = -1;
    var writingReset = -1;
    var activeTimer = 0;
    var resetTimer = 0;
    var resetCB = ()=> {};

    _this.active = 0;

    _this.onUpdateCount = ()=> {
      console.log(`There are ${numBoards} boards`);
    };

    var errCheck = (data)=> {
      let tot = 0;
      for (let i = 0; i < data.length - 1; i++) {
        tot += data[i];
      }

      return ((tot & 0b01111111) == data[data.length - 1]);
    };

    serial.onMessage = (data)=> {
      if (errCheck(data)) {
        let addr = data[0] & 0b01111111;
        let cmd = data[1];
        if (addr == 126) {
          console.log(data.toString().substr(1));
        }

        switch (cmd){
          case SEND_COUNT:
            if (countRequested) {
              countRequested = false;
              numBoards = data[2];
              for (let j = 0; j < numBoards; j++) {
                _this.states[j] = [];
              }

              _this.onUpdateCount();
            }

            break;
          case SET_COLUMN_ACTIVE:
            if (writingActive == addr) {
              //console.log('Finished setting active');
              _this.active = writingActive;
              writingActive = 0;
              clearTimeout(activeTimer);
            }

            break;
          case RESET_COLUMN:
            if (writingReset == addr || addr == BROADCAST) {
              _this.active = 0;
              resetCB();
              writingReset = 0;
              clearTimeout(resetTimer);
            }

            break;
          case GET_STATES: {
            addr -= 1;
            let states = 0;
            let numCells = 0;
            numCells = data[2];
            console.log(`there are ${numCells} cells in column ${addr}`);
            if (numCells && numBoards) {
              for (let j = 0; j  < (numCells / 7); j++) {
                states = data[3 + j];
                console.log(states);
                for (let k = 0; k < 7 && k + j * 7 < numCells; k++) {
                  let newRead = (states & Math.pow(2, k));
                  if (_this.states[addr][j * 7 + k] != newRead) {
                    _this.states[addr][j * 7 + k] = newRead;
                    _this.onCellChange(addr, k + j * 7, newRead);
                  }
                }
              }
            }
          }

            break;
          case ROW_COLOR: {
            console.log(data);
          }

            break;
        }
      } else {
        var str = data.toString().substr(1);
        if (str.toLowerCase().includes('error')) console.error(str);
        else console.log(str);
      }
    };

    var sendPacket = (arr, print)=> {
      arr[0] |= 0b10000000;
      arr.push(arr.reduce((acc, val)=>acc + val, 0) & 0b01111111);
      arr.push(COMMAND + END_FLAG);
      // console.log('----------------- Sent ---------------');
      if (print) console.log(arr);
      serial.send(arr, print);
    };

    _this.onReady = ()=> {};

    _this.onCellChange = (col, row, val)=> {
      console.log(`row ${row}, column ${col} changed`);
    };

    _this.getColumnCount = ()=> {
      sendPacket([BROADCAST, SEND_COUNT, 0]);
    };

    _this.setRowColor = (which, r, g, b)=> {
      sendPacket([BROADCAST, ROW_COLOR, which, r, g, b]);
    };

    serial.onOpen = () => {
      console.log('Started serial');

      setTimeout(()=> {
        _this.getColumnCount();

        _this.onReady();
      }, 2000);
    };

    let requestInterval = null;

    let resetActive = (which, cb)=> {
      if (which > 0) {
        writingReset = which;
        resetCB = cb;

        //resetTimer = setTimeout(()=> {resetActive(which);}, 50);
        //console.log('reset active');
        sendPacket([which, RESET_COLUMN]);
      } else cb();

    };

    let setActive = (which)=> {
      writingActive = which;

      //activeTimer = setTimeout(()=> {setActive(which);}, 50);
      //console.log(`Setting ${which} active`);
      sendPacket([which, SET_COLUMN_ACTIVE]);
    };

    _this.onNextActive = ()=> {};

    _this.setNextActive = ()=> {
      let oldActive = _this.active;
      resetActive(BROADCAST, ()=> {
        //_this.getColumnCount();
        setActive((oldActive % numBoards) + 1);
      });
      //

      _this.onNextActive(_this.states[oldActive % numBoards], oldActive % numBoards);
    };

    _this.setup = ()=> {
      serial.open({ manufacturer: 'FTDI', baud: 115200 });
    };

  };
});
