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

  const fuelTable = $('#fuelTable');
  const newFuelRow = $('#newFuelRow');
  const removeFuel = (index) => {
    if (opt !== null)
      return;
    fuelTable.children().eq(index).remove();
    const rows = fuelTable.children();
    for (let i = index; i < rows.length - 1; ++i)
      rows.eq(i).children().first().text(i + 1);
  };
  const addFuel = () => {
    if (opt !== null)
      return;
    const row = $('<tr></tr>');
    row.append('<td>' + fuelTable.children().length + '</td>');
    row.append('<td><input type="text" class="name"></td>');
    row.append('<td><input type="text" class="efficiency"></td>');
    row.append('<td><input type="text" class="heat"></td>');
    row.append('<td><input type="text" class="criticality"></td>');
    row.append('<td><input type="checkbox" class="selfPriming"></td>');
    row.append('<td><input type="text" class="limit"></td>');
    row.append('<td><a href="javascript:;" class="del">Del</a></td>');
    row.insertBefore(newFuelRow);
    row.find('.del').click(() => removeFuel(row.index()));
    return row;
  };
  const fuelPresets = {};
  const fuelPresetHead = $('#fuelPresets tr').first();
  const fuelPresetContent = $('#fuelPresets tr').last();
  $('#newFuel').click(addFuel);
  const addFuelPreset = (type, fuel, efficiency, heat, criticality, selfPriming) => {
    const link = $('<a href="javascript:;">' + type + '</a>');
    link.click(() => {
      if (opt !== null)
        return;
      let name = '[' + type + ']';
      if (fuel.substr(0, 3) == "MIX")
        name += 'M' + type + fuel.substr(3);
      else
        name += fuel;
      const row = addFuel();
      row.find('.name').val(name);
      row.find('.efficiency').val(efficiency);
      row.find('.heat').val(heat);
      row.find('.criticality').val(criticality);
      row.find('.selfPriming').prop('checked', selfPriming);
    });
    if (!fuelPresets.hasOwnProperty(fuel)) {
      fuelPresetHead.append('<td>' + fuel + '</td>')
      const content = $('<td></td>');
      content.append(link);
      fuelPresetContent.append(content);
      fuelPresets[fuel] = content;
    } else {
      fuelPresets[fuel].append(' ').append(link);
    }
  };
  addFuelPreset("OX", "TBU",      125, 40, 234);
  addFuelPreset("OX", "LEU-233",  110, 216, 78);
  addFuelPreset("OX", "HEU-233",  115, 648, 39);
  addFuelPreset("OX", "LEU-235",  100, 120, 102);
  addFuelPreset("OX", "HEU-235",  105, 360, 51);
  addFuelPreset("OX", "LEN-236",  110, 292, 70);
  addFuelPreset("OX", "HEN-236",  115, 876, 35);
  addFuelPreset("OX", "LEP-239",  120, 126, 99);
  addFuelPreset("OX", "HEP-239",  125, 378, 49);
  addFuelPreset("OX", "LEP-241",  125, 182, 84);
  addFuelPreset("OX", "HEP-241",  130, 546, 42);
  addFuelPreset("OX", "MIX-239",  105, 132, 94);
  addFuelPreset("OX", "MIX-241",  115, 192, 80);
  addFuelPreset("OX", "LEA-242",  135, 390, 65);
  addFuelPreset("OX", "HEA-242",  140, 1170, 32);
  addFuelPreset("OX", "LECm-243", 145, 384, 66);
  addFuelPreset("OX", "HECm-243", 150, 1152, 33);
  addFuelPreset("OX", "LECm-245", 150, 238, 75);
  addFuelPreset("OX", "HECm-245", 155, 714, 37);
  addFuelPreset("OX", "LECm-247", 155, 268, 72);
  addFuelPreset("OX", "HECm-247", 160, 804, 36);
  addFuelPreset("OX", "LEB-248",  165, 266, 73);
  addFuelPreset("OX", "HEB-248",  170, 798, 36);
  addFuelPreset("OX", "LECf-249", 175, 540, 60, true);
  addFuelPreset("OX", "HECf-249", 180, 1620, 30, true);
  addFuelPreset("OX", "LECf-251", 180, 288, 71, true);
  addFuelPreset("OX", "HECf-251", 185, 864, 35, true);
  addFuelPreset("NI", "TBU",      125, 32, 293);
  addFuelPreset("NI", "LEU-233",  110, 172, 98);
  addFuelPreset("NI", "HEU-233",  115, 516, 49);
  addFuelPreset("NI", "LEU-235",  100, 96, 128);
  addFuelPreset("NI", "HEU-235",  105, 288, 64);
  addFuelPreset("NI", "LEN-236",  110, 234, 88);
  addFuelPreset("NI", "HEN-236",  115, 702, 44);
  addFuelPreset("NI", "LEP-239",  120, 100, 124);
  addFuelPreset("NI", "HEP-239",  125, 300, 62);
  addFuelPreset("NI", "LEP-241",  125, 146, 105);
  addFuelPreset("NI", "HEP-241",  130, 438, 52);
  addFuelPreset("NI", "MIX-239",  105, 106, 118);
  addFuelPreset("NI", "MIX-241",  115, 154, 100);
  addFuelPreset("NI", "LEA-242",  135, 312, 81);
  addFuelPreset("NI", "HEA-242",  140, 936, 40);
  addFuelPreset("NI", "LECm-243", 145, 308, 83);
  addFuelPreset("NI", "HECm-243", 150, 924, 41);
  addFuelPreset("NI", "LECm-245", 150, 190, 94);
  addFuelPreset("NI", "HECm-245", 155, 570, 47);
  addFuelPreset("NI", "LECm-247", 155, 214, 90);
  addFuelPreset("NI", "HECm-247", 160, 642, 45);
  addFuelPreset("NI", "LEB-248",  165, 212, 91);
  addFuelPreset("NI", "HEB-248",  170, 636, 45);
  addFuelPreset("NI", "LECf-249", 175, 432, 75, true);
  addFuelPreset("NI", "HECf-249", 180, 1296, 37, true);
  addFuelPreset("NI", "LECf-251", 180, 230, 89, true);
  addFuelPreset("NI", "HECf-251", 185, 690, 44, true);
  addFuelPreset("ZA", "TBU",      125, 50, 199);
  addFuelPreset("ZA", "LEU-233",  110, 270, 66);
  addFuelPreset("ZA", "HEU-233",  115, 810, 33);
  addFuelPreset("ZA", "LEU-235",  100, 150, 87);
  addFuelPreset("ZA", "HEU-235",  105, 450, 43);
  addFuelPreset("ZA", "LEN-236",  110, 366, 60);
  addFuelPreset("ZA", "HEN-236",  115, 1098, 30);
  addFuelPreset("ZA", "LEP-239",  120, 158, 84);
  addFuelPreset("ZA", "HEP-239",  125, 474, 42);
  addFuelPreset("ZA", "LEP-241",  125, 228, 71);
  addFuelPreset("ZA", "HEP-241",  130, 684, 35);
  addFuelPreset("ZA", "MIX-239",  105, 166, 80);
  addFuelPreset("ZA", "MIX-241",  115, 240, 68);
  addFuelPreset("ZA", "LEA-242",  135, 488, 55);
  addFuelPreset("ZA", "HEA-242",  140, 1464, 27);
  addFuelPreset("ZA", "LECm-243", 145, 480, 56);
  addFuelPreset("ZA", "HECm-243", 150, 1440, 28);
  addFuelPreset("ZA", "LECm-245", 150, 298, 64);
  addFuelPreset("ZA", "HECm-245", 155, 894, 32);
  addFuelPreset("ZA", "LECm-247", 155, 336, 61);
  addFuelPreset("ZA", "HECm-247", 160, 1008, 30);
  addFuelPreset("ZA", "LEB-248",  165, 332, 62);
  addFuelPreset("ZA", "HEB-248",  170, 996, 31);
  addFuelPreset("ZA", "LECf-249", 175, 676, 51, true);
  addFuelPreset("ZA", "HECf-249", 180, 2028, 25, true);
  addFuelPreset("ZA", "LECf-251", 180, 360, 60, true);
  addFuelPreset("ZA", "HECf-251", 185, 1080, 30, true);
  const Air = 40, C0 = 41, M0 = 32, R0 = 35, Shield = 37, Irradiator = 38, Conductor = 39;
  const cellSources = [];
  const tileNames = [
    'Wt', 'Fe', 'Rs', 'Qz', 'Ob', 'Nr', 'Gs', 'Lp', 'Au', 'Pm', 'Sm', 'En', 'Pr', 'Dm', 'Em', 'Cu',
    'Sn', 'Pb', 'B',  'Li', 'Mg', 'Mn', 'Al', 'Ag', 'Fl', 'Vi', 'Cb', 'As', 'N',  'He', 'Ed', 'Cr',
    '##', '==', '--', '=)', '-)', '<>', '><', '[]', '..'];
  const tileTitles = [
    'Water', 'Iron', 'Redstone', 'Quartz', 'Obsidian', 'Nether Bricks', 'Glowstone', 'Lapis', 'Gold', 'Prismarine',
    'Slime', 'End Stone', 'Purpur', 'Diamond', 'Emerald', 'Copper', 'Tin', 'Lead', 'Boron', 'Lithium', 'Magnesium',
    'Manganese', 'Aluminum', 'Silver', 'Fluorite', 'Villiaumite', 'Carobbiite', 'Arsenic', 'Nitrogen', 'Helium',
    'Enderium', 'Cryotheum', 'Graphite', 'Beryllium', 'Heavy Water', 'Beryllium-Carbon', 'Lead-Steel', 'Boron-Silver',
    'Irradiator', 'Conductor', 'Air'];
  const tileClasses = tileNames.slice();
  tileClasses[M0] = 'M0';
  tileClasses[M0 + 1] = 'M1';
  tileClasses[M0 + 2] = 'M2';
  tileClasses[R0] = 'R0';
  tileClasses[R0 + 1] = 'R1';
  tileClasses[Shield] = 'other';
  tileClasses[Irradiator] = 'other';
  tileClasses[Conductor] = 'other';
  tileClasses[Air] = 'air';
  const tileSaveNames = tileTitles.slice();
  tileSaveNames[5] = 'NetherBrick';
  tileSaveNames[11] = 'EndStone';
  tileSaveNames[M0 + 2] = 'HeavyWater';
  tileSaveNames[Irradiator] = "{\"HeatPerFlux\":0,\"EfficiencyMultiplier\":0.0}";
  const displayTile = (tile, pad) => {
    const name = tileNames[tile];
    const result = $('<span>' + name + '</span>').addClass(tileClasses[tile]);
    if (pad && name.length == 1)
      result.append($('<span>.</span>').addClass('air'));
    result.attr('title', tileTitles[tile]);
    return result;
  };
  const blockTypes = $('#blockTypes');
  const blockLimits = $('#blockLimits');
  for (let i = 0; i < tileNames.length - 1; ++i) {
    blockTypes.append($('<td></td>').append(displayTile(i, false)));
    blockLimits.append('<td><input type="text" class="limit"></td>');
  }

  const schedule = () => {
    timeout = window.setTimeout(step, 0);
  };

  const settings = new FissionOpt.OverhaulFissionSettings();
  const design = $('#design');
  const save = $('#save');

  const displaySample = (sample) => {
    design.empty();
    let block = $('<div></div>');
    const appendInfo = (label, value, unit) => {
      const row = $('<div></div>').addClass('info');
      row.append('<div>' + label + '</div>');
      row.append('<div>' + unit + '</div>');
      row.append(Math.round(value * 100) / 100);
      block.append(row);
    };
    appendInfo('Output', sample.getOutput() / 16, 'mB/t');
    appendInfo('Efficiency', sample.getEfficiency() * 100, '%');
    appendInfo('Fuel Use', sample.getFuelUse(), '&times;');
    appendInfo('Irr. Flux', sample.getIrradiatorFlux(), 'N');
    design.append(block);

    const shapes = [], strides = [], data = sample.getData();
    for (let i = 0; i < 3; ++i) {
      shapes.push(sample.getShape(i));
      strides.push(sample.getStride(i));
    }

    let resourceMap = {};
    resourceMap[-1] = (shapes[0] * shapes[1] + shapes[1] * shapes[2] + shapes[2] * shapes[0]) * 2 + (shapes[0] + shapes[1] + shapes[2]) * 4 + 8;
    const increaseResource = (key) => {
      if (!resourceMap.hasOwnProperty(key))
        resourceMap[key] = 1;
      else
        ++resourceMap[key];
    };

    const saved = {
      SaveVersion: {Major: 2, Minor: 1, Build: 4, Revision: 0, MajorRevision: 0, MinorRevision: 0},
      Data: {
        InteriorDimensions: {X: shapes[2], Y: shapes[0], Z: shapes[1]},
        CoolantRecipeName: "Water to High Pressure Steam",
        HeatSinks: {},
        Moderators: {},
        Reflectors: {},
        NeutronShields: {},
        Irradiators: {},
        Conductors: [],
        FuelCells: {}
      }
    };
    const saveTile = (tile, x, y, z) => {
      let category;
      if (tile < M0) {
        category = saved.Data.HeatSinks;
      } else if (tile < R0) {
        category = saved.Data.Moderators;
      } else if (tile < Shield) {
        category = saved.Data.Reflectors;
      } else if (tile == Shield) {
        category = saved.Data.NeutronShields;
      } else if (tile == Irradiator) {
        category = saved.Data.Irradiators;
      } else if (tile == Conductor) {
        category = saved.Data.Conductors;
      } else if (tile == Air) {
        return;
      } else {
        category = saved.Data.FuelCells;
      }
      if (tile != Conductor) {
        const name = tileSaveNames[tile];
        if (!category.hasOwnProperty(name))
          category[name] = [];
        category = category[name];
      }
      category.push({X: z + 1, Y: x + 1, Z: y + 1});
    };

    for (let x = 0; x < shapes[0]; ++x) {
      block = $('<div></div>');
      block.append('<div>Layer ' + (x + 1) + '</div>');
      for (let y = 0; y < shapes[1]; ++y) {
        const row = $('<div></div>').addClass('row');
        for (let z = 0; z < shapes[2]; ++z) {
          if (z)
            row.append(' ');
          const tile = data[x * strides[0] + y * strides[1] + z * strides[2]];
          row.append(displayTile(tile, true));
          if (tile < Air) {
            increaseResource(tile);
          } else if (tile >= C0) {
            increaseResource(-2);
            const source = cellSources[tile - C0];
            if (source)
              increaseResource(-2 - source);
          }
          saveTile(tile, x, y, z);
        }
        block.append(row);
      }
      design.append(block);
    }

    save.removeClass('disabledLink');
    save.off('click').click(() => {
      const elem = document.createElement('a');
      const url = window.URL.createObjectURL(new Blob([JSON.stringify(saved)], {type: 'text/json'}));
      elem.setAttribute('href', url);
      elem.setAttribute('download', 'reactor.json');
      elem.click();
      window.URL.revokeObjectURL(url);
    });

    block = $('<div></div>');
    block.append('<div>Total number of blocks used</div>')
    resourceMap = Object.entries(resourceMap);
    resourceMap.sort((x, y) => y[1] - x[1]);
    for (resource of resourceMap) {
      const row = $('<div></div>');
      if (resource[0] == -1)
        row.append('Casing');
      else if (resource[0] == -2)
        row.append('Cell');
      else if (resource[0] == -3)
        row.append('Cf-252');
      else if (resource[0] == -4)
        row.append('Po-Be');
      else if (resource[0] == -5)
        row.append('Ra-Be');
      else
        row.append(displayTile(resource[0], false).addClass('row'));
      block.append(row.append(' &times; ' + resource[1]));
    }
    design.append(block);
  };

  const progress = $('#progress');
  let lossElement, lossPlot;
  function step() {
    schedule();
    opt.stepInteractive();
    const nStage = opt.getNStage();
    if (nStage == 1)
      progress.text('Episode ' + opt.getNEpisode() + ', training iteration ' + opt.getNIteration());
    else if (nStage == 2)
      progress.text('Episode ' + opt.getNEpisode() + ', inference iteration ' + opt.getNIteration());
    else
      progress.text('Episode ' + opt.getNEpisode() + ', rollout iteration ' + opt.getNIteration());
    if (opt.needsRedrawBest())
      displaySample(opt.getBest());
    if (opt.needsReplotLoss()) {
      const data = opt.getLossHistory();
      while (lossPlot.data.labels.length < data.length)
        lossPlot.data.labels.push(lossPlot.data.labels.length);
      lossPlot.data.datasets[0].data = data;
      lossPlot.update({duration: 0});
    }
  };

  run.click(() => {
    if (timeout !== null)
      return;
    if (opt === null) {
      const parsePositiveFloat = (name, x) => {
        const result = parseFloat(x);
        if (!(result > 0))
          throw Error(name + " must be a positive number.");
        return result;
      };
      const parsePositiveInt = (name, x) => {
        const result = parseInt(x);
        if (!(result > 0))
          throw Error(name + " must be a positive integer.");
        return result;
      };
      const parseLimit = (x) => {
        x = parseInt(x);
        return x >= 0 ? x : -1;
      }
      try {
        settings.sizeX = parsePositiveInt('Interior size', $('#sizeX').val());
        settings.sizeY = parsePositiveInt('Interior size', $('#sizeY').val());
        settings.sizeZ = parsePositiveInt('Interior size', $('#sizeZ').val());
        settings.clearFuels();
        while (cellSources.length) {
          cellSources.pop();
          tileNames.pop();
          tileTitles.pop();
          tileClasses.pop();
          tileSaveNames.pop();
        }
        const fuels = fuelTable.children();
        if (fuels.length == 1)
          throw Error("No fuel.");
        for (let i = 0; i < fuels.length - 1; ++i) {
          const fuel = fuels.eq(i);
          const selfPriming = fuel.find('.selfPriming').is(':checked');
          const name = fuel.find('.name').val();
          settings.addFuel(
            parsePositiveFloat('Efficiency', fuel.find('.efficiency').val()) / 100,
            parseLimit(fuel.find('.limit').val()),
            parsePositiveInt('Criticality', fuel.find('.criticality').val()),
            parsePositiveInt('Heat', fuel.find('.heat').val()),
            selfPriming);
          cellSources.push(0);
          tileClasses.push('other');
          if (selfPriming) {
            tileNames.push(i + 1 + 'S');
            tileTitles.push('Cell for Fuel #' + (i + 1) + ', Self-Primed');
            tileSaveNames.push(name + ';True;Self');
          } else {
            tileNames.push((i + 1).toString());
            tileTitles.push('Cell for Fuel #' + (i + 1));
            tileSaveNames.push(name + ';False;None');

            tileClasses.push('other');
            cellSources.push(1);
            tileNames.push(i + 1 + 'A');
            tileTitles.push('Cell for Fuel #' + (i + 1) + ', Primed by Cf-252');
            tileSaveNames.push(name + ';True;Cf-252');

            tileClasses.push('other');
            cellSources.push(2);
            tileNames.push(i + 1 + 'B');
            tileTitles.push('Cell for Fuel #' + (i + 1) + ', Primed by Po-Be');
            tileSaveNames.push(name + ';True;Po-Be');

            tileClasses.push('other');
            cellSources.push(3);
            tileNames.push(i + 1 + 'C');
            tileTitles.push('Cell for Fuel #' + (i + 1) + ', Primed by Ra-Be');
            tileSaveNames.push(name + ';True;Ra-Be');
          }
        }
        blockLimits.find('input').each(function(i) {
          settings.setLimit(i, parseLimit($(this).val()));
        });
        for (let i = 0; i < 3; ++i)
          settings.setSourceLimit(i, parseLimit($('#sourceLimit' + i).val()));
        settings.goal = parseInt($('input[name=goal]:checked').val());
        settings.controllable = $('#controllable').is(':checked');
        settings.symX = $('#symX').is(':checked');
        settings.symY = $('#symY').is(':checked');
        settings.symZ = $('#symZ').is(':checked');
      } catch (error) {
        alert('Error: ' + error.message);
        return;
      }
      design.empty();
      save.off('click');
      save.addClass('disabledLink');
      if (lossElement !== undefined)
        lossElement.remove();
      lossElement = $('<canvas></canvas>').attr('width', 1024).attr('height', 128).insertAfter(progress);
      lossPlot = new Chart(lossElement[0].getContext('2d'), {
        type: 'bar',
        options: {responsive: false, animation: {duration: 0}, hover: {animationDuration: 0}, scales: {xAxes: [{display: false}]}, legend: {display: false}},
        data: {labels: [], datasets: [{label: 'Loss', backgroundColor: 'red', data: [], categoryPercentage: 1.0, barPercentage: 1.0}]}
      });
      opt = new FissionOpt.OverhaulFissionOpt(settings);
    }
    schedule();
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
