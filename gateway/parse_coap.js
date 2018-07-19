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
