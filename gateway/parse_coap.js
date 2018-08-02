/* Parse Permamote advertisements */

var parse_payload = function (device_id, resource_url, payload, cb) {
  if(device_id != null) {
    if (device_id.slice(6, 8) === '11') {
      var out = {
        device: 'Permamote',
      }
      // OK! This looks like a Permamote packet
      if (resource_url === '/light_lux') {
        out.light_lux = payload.readFloatLE();
      }
      else if (resource_url === '/motion') {
        out.motion = payload[0];
      }
      else if (resource_url === '/temperature_c') {
        out.temperature_c = payload.readFloatLE();
      }
      else if (resource_url === '/pressure_mbar') {
        out.pressure_mbar = payload.readFloatLE();
      }
      else if (resource_url === '/humidity_percent') {
        out.humidity_percent = payload.readFloatLE();
      }
      else if (resource_url === '/voltage') {
        out.primary_voltage = payload.readFloatLE();
        out.solar_voltage = payload.readFloatLE(4);
        out.secondary_voltage = payload.readFloatLE(8);
      }
      else if (resource_url === '/error') {
        out.error = payload.readUInt32LE();
      }
      else {
        cb(null);
        return;
      }
      //var out = {
      //  device: 'Permamote',
      //  light_lux: light,
      //  light_red: color_red,
      //  light_green: color_green,
      //  light_blue: color_blue,
      //  primary_voltage: vbat,
      //  solar_voltage: vsol,
      //  secondary_voltage: vsec,
      //  sequence_number: sequence_num,
      //};

      cb(out);
      return;
    }

    cb(null);
  }
}


module.exports = {
    parsePayload: parse_payload
};