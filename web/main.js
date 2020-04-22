$(() => { FissionOpt().then((FissionOpt) => {
  const run = $('#run'), pause = $('#pause'), stop = $('#stop');
  let opt = null, timeout = null;
  
  const updateDisables = () => {
    $('#settings input').prop('disabled', opt !== null);
    $('#settings a')[opt === null ? 'removeClass' : 'addClass']('disabledLink');
    run[timeout === null ? 'removeClass' : 'addClass']('disabledLink');
    pause[timeout !== null ? 'removeClass' : 'addClass']('disabledLink');
    stop[opt !== null ? 'removeClass' : 'addClass']('disabledLink');
  };

  const fuelBasePower = $('#fuelBasePower');
  const fuelBaseHeat = $('#fuelBaseHeat');
  const fuelPresets = {
    LEU235Default: [120, 50],
    MOX239Default: [155.4, 57.5],
    LEU235E2E: [720, 60],
    MOX239E2E: [932.4, 69],
    LEU235PO3: [36000, 200],
    MOX239PO3: [46512, 230]
  };
  for (const [name, [power, heat]] of Object.entries(fuelPresets)) {
    $('#' + name).click(() => {
      if (opt !== null)
        return;
      fuelBasePower.val(power);
      fuelBaseHeat.val(heat);
    });
  }
  const applyFuelFactor = (factor) => {
    if (opt !== null)
      return;
    fuelBasePower.val(fuelBasePower.val() * factor);
    fuelBaseHeat.val(fuelBaseHeat.val() * factor);
  };
  $('#br').click(() => { applyFuelFactor(8 / 9); });
  $('#ic2').click(() => { applyFuelFactor(18 / 19); });
  $('#ic2mox').click(() => { applyFuelFactor(9 / 7); });
  
  const rates = [], allows = [];
  $('#rate input').each(function() { rates.push($(this)); });
  $('#allow input').each(function() { allows.push($(this)); });
  const loadRatePreset = (preset) => {
    if (opt !== null)
      return;
    $.each(rates, (i, x) => { x.val(preset[i]); });
  };
  $('#coolerDefault').click(() => { loadRatePreset([60, 90, 90, 120, 130, 120, 150, 140, 120, 160, 80, 160, 80, 120, 110]); });
  $('#coolerE2E').click(() => { loadRatePreset([20, 80, 80, 120, 120, 100, 120, 120, 140, 140, 60, 140, 60, 80, 100]); });
  $('#coolerPO3').click(() => { loadRatePreset([40, 160, 160, 240, 240, 200, 240, 240, 280, 800, 120, 280, 120, 160, 200]); });

  const scheduleBatch = () => {
    timeout = window.setTimeout(runBatch, 0);
  };

  const normal = $('#normal'), noNetHeat = $('#noNetHeat');
  const tileNames = ['Wt', 'Rs', 'Qz', 'Au', 'Gs', 'Lp', 'Dm', 'He', 'Ed', 'Cr', 'Fe', 'Em', 'Cu', 'Sn', 'Mg', '&nbsp;&nbsp;', '[]', '##'];
  const tileClasses = tileNames.slice();
  tileClasses[15] = 'row';
  tileClasses[16] = 'cell';
  tileClasses[17] = 'mod';
  const displayDesign = (design, element) => {
    element.children(':not(:first)').remove();
    const appendInfo = (label, value, unit) => {
      const row = $('<div></div>').addClass('info');
      row.append('<div>' + label + '</div>');
      row.append('<div>' + unit + '</div>');
      row.append(Math.round(value * 100) / 100);
      element.append(row);
    };
    appendInfo('Max Power', design.getPower(), 'RF/t');
    appendInfo('Heat', design.getHeat(), 'H/t');
    appendInfo('Cooling', design.getCooling(), 'H/t');
    appendInfo('Net Heat', design.getNetHeat(), 'H/t');
    appendInfo('Duty Cycle', design.getDutyCycle() * 100, '%');
    appendInfo('Avg Power', design.getEffPower(), 'RF/t');
    element.append('<div></div>')
    const shapes = [], strides = [], data = design.getData();
    for (let i = 0; i < 3; ++i) {
      shapes.push(design.getShape(i));
      strides.push(design.getStride(i));
    }
    for (let x = 0; x < shapes[0]; ++x) {
      element.append($('<div></div>').addClass('layerSpacer'));
      element.append('<div>Layer ' + (x + 1) + '</div>');
      for (let y = 0; y < shapes[1]; ++y) {
        const row = $('<div></div>').addClass('row');
        for (let z = 0; z < shapes[2]; ++z) {
          if (z)
            row.append('&nbsp;');
          const tile = data[x * strides[0] + y * strides[1] + z * strides[2]];
          row.append($('<span>' + tileNames[tile] + '</span>').addClass(tileClasses[tile]));
        }
        element.append(row);
      }
    }
    element.removeClass('hidden');
  };

  function runBatch() {
    scheduleBatch();
    let whichChanged = 0;
    for (let i = 0; i < 1024; ++i)
      whichChanged |= opt.step();
    if (whichChanged & 1)
      displayDesign(opt.getBest(), normal);
    if (whichChanged & 2)
      displayDesign(opt.getBestNoNetHeat(), noNetHeat)
  };

  const settings = new FissionOpt.Settings();
  run.click(() => {
    if (timeout !== null)
      return;
    if (opt === null) {
      const parseSize = (x) => {
        const result = parseInt(x);
        if (!(result > 0))
          throw Error("Core size must be a positive integer");
        return result;
      };
      const parsePositiveFloat = (name, x) => {
        const result = parseFloat(x);
        if (!(result > 0))
          throw Error(name + " must be a positive number");
        return result;
      };
      try {
        settings.sizeX = parseSize($('#sizeX').val());
        settings.sizeY = parseSize($('#sizeY').val());
        settings.sizeZ = parseSize($('#sizeZ').val());
        settings.fuelBasePower = parsePositiveFloat('Fuel Base Power', fuelBasePower.val());
        settings.fuelBaseHeat = parsePositiveFloat('Fuel Base Heat', fuelBaseHeat.val());
        $.each(rates, (i, x) => { settings.setRate(i, parsePositiveFloat('Cooling Rate', x.val())); });
        $.each(allows, (i, x) => { settings.setAllow(i, x.is(':checked')); });
      } catch (error) {
        alert('Error: ' + error.message);
        return;
      }
      opt = new FissionOpt.OptMeta(settings);
    }
    scheduleBatch();
    updateDisables();
  });

  pause.click(() => {
    if (timeout === null)
      return;
    window.clearTimeout(timeout);
    timeout = null;
    updateDisables();
  });

  stop.click(() => {
    if (opt === null)
      return;
    if (timeout !== null) {
      window.clearTimeout(timeout);
      timeout = null;
    }
    opt.delete();
    opt = null;
    updateDisables();
  });
}); });
