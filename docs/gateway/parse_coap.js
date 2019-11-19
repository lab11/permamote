/* Parse Permamote advertisements */

var parse_payload = function (device_id, resource_url, payload, cb) {
  if(device_id != null) {
    if (device_id.slice(6, 8) === '11') {
      topic = resource_url.split('/')
      topic = topic[topic.length - 1]
      var out = {
        device: 'Permamote',
        topic: topic,
      }
      // OK! This looks like a Permamote packet
      if (resource_url == '/version') {
        out.git_version = payload.toString('utf-8');
      }
      else if (resource_url == '/thread_info') {
        out.rloc16 = payload.readUInt16LE();
        out.ext_addr = payload.toString('hex', 2, 10);
      }
      else if (resource_url === '/light_lux') {
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
      else if (resource_url == '/light_color_cct_k') {
        out.light_cct_k = payload.readFloatLE();
        out.topic = 'light_cct_k'
      }
      else if (resource_url === '/light_color_counts') {
        out.light_red   = payload.readUInt16LE();
        out.light_green = payload.readUInt16LE(2);
        out.light_blue  = payload.readUInt16LE(4);
        out.light_clear = payload.readUInt16LE(6);
      }
      else if (resource_url === '/free_ot_buffers') {
        out.free_ot_buffers = payload.readUInt16LE();
      }
      else if (resource_url === '/vbat_ok') {
        out.vbat_ok = payload[0];
      }
      else if (resource_url === '/weight_mg') {
        out.weight_mg = payload.readInt32LE();
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
