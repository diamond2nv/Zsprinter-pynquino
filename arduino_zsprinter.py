from . import Arduino 


__author__ = "S.Aoki"
__copyright__ = "Copyright 2018, Shokara"
__email__ = "aoki@shokara.com"

ARDUINO_OLED_PROGRAM = "arduino_zsprinter.bin"
PRINT_STRING = 0x3
#START_COMMAND_DUMMY = 0xA
BUFLEN_DATA_ADDR = 100
X_DATA_ADDR = 101
Y_DATA_ADDR = 102
Z_DATA_ADDR = 103
E_DATA_ADDR = 104
TEMP_DATA_ADDR = 105
ENDSTOP_X_ADDR = 106
ENDSTOP_Y_ADDR = 107
ENDSTOP_Z_ADDR = 108
BUFLEN_ACCUM_DATA_ADDR = 109
BUFLEN_ACCUM_FINISHED_DATA_ADDR = 110
NEXT_BUFFER_HEAD_ADDR = 111

class Arduino_Zsprinter(object):
    """This class controls an Zsprinter.
    
    Attributes
    ----------
    microblaze : Arduino
        Microblaze processor instance used by this module.

    """

    def __init__(self, mb_info):
        """Return a new instance of an OLED object. 
        
        Parameters
        ----------
        mb_info : dict
            A dictionary storing Microblaze information, such as the
            IP name and the reset name.
        text: str
            The text to be displayed after initialization.
            
        """
        self.microblaze = Arduino(mb_info, ARDUINO_OLED_PROGRAM)
            
    def write(self, text, x=0, y=0):
        """Write a new text string on the OLED.

        Parameters
        ----------
        text : str
            The text string to be displayed on the OLED screen.
        x : int
            The x-position of the display.
        y : int
            The y-position of the display.

        Returns
        -------
        None

        """
        if not 0 <= x <= 255:
            raise ValueError("X-position should be in [0, 255]")
        if not 0 <= y <= 255:
            raise ValueError("Y-position should be in [0, 255]")
        if len(text) >= 64:
            #raise ValueError("text too long to be displayed.")
            print("Text too long and trimmed")
            text = text[:64]

        # First write length, x, y, then write rest of string
        data = [len(text), x, y]
        data += [ord(char) for char in text]
        self.microblaze.write_mailbox(0, data)

        # Finally write the print string command
        self.microblaze.write_blocking_command(PRINT_STRING)

    #def start(self):
    #    self.microblaze.write_blocking_command(START_COMMAND_DUMMY)

    def read_buflen(self):
        buflen = self.microblaze.read_mailbox(4*BUFLEN_DATA_ADDR,4)
        return buflen

    def read_buflen_accum(self):
        buflen_accum = self.microblaze.read_mailbox(4*BUFLEN_ACCUM_DATA_ADDR,4)
        return buflen_accum

    def read_buflen_accum_finished(self):
        buflen_accum_finished = self.microblaze.read_mailbox(4*BUFLEN_ACCUM_FINISHED_DATA_ADDR,4)
        return buflen_accum_finished

    def read_next_buffer_head(self):
        next_buffer_head = self.microblaze.read_mailbox(4*NEXT_BUFFER_HEAD_ADDR,4)
        return next_buffer_head 

