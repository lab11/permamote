/* Parse Permamote advertisements */

var parse_advertisement = function (advertisement, cb) {

    if (advertisement.localName === 'Permamote') {
        if (advertisement.manufacturerData) {
            // Need at least 3 bytes. Two for manufacturer identifier and
            // one for the service ID.
            if (advertisement.manufacturerData.length >= 3) {
                // Check that manufacturer ID and service byte are correct
                var manufacturer_id = advertisement.manufacturerData.readUIntLE(0, 2);
                var service_id = advertisement.manufacturerData.readUInt8(2);
                if (manufacturer_id == 0x02E0 && service_id == 0x20) {
                    // OK! This looks like a Permamote packet
                    if (advertisement.manufacturerData.length >= 19) {
                        var sensor_data = advertisement.manufacturerData.slice(3);

                        var light_exp =   sensor_data.readUInt8(0);
                        var light_mant =  sensor_data.readUInt8(1);
                        var light = (1 << light_exp) * light_mant * 0.045
                        var color_red =   sensor_data.readUInt16LE(2);
                        var color_green = sensor_data.readUInt16LE(4);
                        var color_blue =  sensor_data.readUInt16LE(6);
                        var vbat = 0.6*6*sensor_data.readUInt16LE(8) / ((1 << 10) - 1);
                        var vsol = 0.6*6*sensor_data.readUInt16LE(10) / ((1 << 10) - 1);
                        var vsec = 0.6*6*sensor_data.readUInt16LE(12) / ((1 << 10) - 1);
                        var sequence_num =  sensor_data.readUInt32LE(14);

                        var out = {
                            device: 'Permamote-ble',
                            light_lux: light,
                            light_red: color_red,
                            light_green: color_green,
                            light_blue: color_blue,
                            primary_voltage: vbat,
                            solar_voltage: vsol,
                            secondary_voltage: vsec,
                            sequence_number: sequence_num,
                        };

                        cb(out);
                        return;
                    }
                }
            }
        }
    }

    cb(null);
}


module.exports = {
    parseAdvertisement: parse_advertisement
};
