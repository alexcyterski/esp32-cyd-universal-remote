"""Static integration checks alongside native_check's battery state tests."""
from pathlib import Path
import unittest
R=Path(__file__).resolve().parents[1]
class Profiles(unittest.TestCase):
 def test_carrier_override(self):
  s=(R/'lib/IRremoteESP8266/src/IRsend.cpp').read_text()
  self.assertIn('duty = PULSE_IR_DUTY;',s)
  self.assertLess(s.index('duty = PULSE_IR_DUTY;'),s.index('_dutycycle = std::min(duty'))
 def test_profile_flags(self):
  s=(R/'platformio.ini').read_text();simple=s.split('[env:e32r28t_simple]')[1].split('[env:e32r28t_full]')[0];full=s.split('[env:e32r28t_full]')[1]
  self.assertIn('PULSE_FULL_BUILD=0',simple);self.assertIn('PULSE_IR_DUTY=50',simple)
  self.assertIn('PULSE_FULL_BUILD=1',full);self.assertIn('PULSE_IR_DUTY=33',full)
 def test_guard_precedes_send(self):
  s=(R/'src/main.cpp').read_text().split('bool emit(')[1].split('void pollReceiver')[0]
  self.assertLess(s.index('batteryGuard.allowed()'),s.index('emitter.sendNEC'))
  self.assertIn('return false;',s[:s.index('emitter.sendNEC')])
if __name__=='__main__':unittest.main()
