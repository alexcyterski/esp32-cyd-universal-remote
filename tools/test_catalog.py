"""Independent known-vector checks for Flipper little-endian conversion."""
import unittest
from build_catalog import convert
class Conversion(unittest.TestCase):
    def parsed(self,p,a,c):
        return convert(dict(type='parsed',protocol=p,address=a,command=c))
    def test_nec_lg_power(self):
        self.assertEqual(self.parsed('NEC','04 00 00 00','08 00 00 00')[:4],(1,32,38,0x20DF10EF))
    def test_samsung_power(self):
        self.assertEqual(self.parsed('Samsung32','07 00 00 00','02 00 00 00')[:4],(2,32,38,0xE0E040BF))
    def test_sony_power(self):
        self.assertEqual(self.parsed('SIRC','01 00 00 00','15 00 00 00')[:4],(3,12,40,0xA90))
    def test_rc5_rc6(self):
        self.assertEqual(self.parsed('RC5','05 00 00 00','0C 00 00 00')[3],0x14C)
        self.assertEqual(self.parsed('RC6','05 00 00 00','0C 00 00 00')[3],0x50C)
    def test_raw_bounds(self):
        with self.assertRaises(ValueError):convert(dict(type='raw',frequency='38000',data='9000 0 560'))
if __name__=='__main__':unittest.main()
