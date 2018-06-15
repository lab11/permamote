
class LightSensor:
    def __init__(self, device_id):
        self.device_id = device_id
        self.light_char_measurements = {}
        self.shade_char_measurements = {}
        self.baseline = 0
        self.lux = 0
        self.last_seq_no = 0
    def __str__(self):
        print_str = self.device_id + '\r\n'
        print_str += 'baseline = ' + str(self.baseline) + '\r\n'
        print_str += str(self.light_char_measurements)
        print_str += str(self.shade_char_measurements)
        return print_str
    def update_seq_no(self, seq_no):
        if seq_no > self.last_seq_no:
            self.last_seq_no = seq_no

