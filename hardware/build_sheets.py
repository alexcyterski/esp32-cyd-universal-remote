from pathlib import Path
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
from reportlab.platypus import Paragraph
from reportlab.lib.styles import ParagraphStyle
from fontTools.ttLib import TTFont as Font
from fontTools.varLib.instancer import instantiateVariableFont

HERE=Path(__file__).resolve().parent
ROOT=HERE.parent if (HERE.parent/'platformio.ini').exists() else HERE/'pulse-remote'
OUT=Path('output/pdf');OUT.mkdir(parents=True,exist_ok=True)
TMP=Path('tmp/hardware-pdfs');TMP.mkdir(parents=True,exist_ok=True)
for name,weight in [('Inter',400),('InterSB',600)]:
 p=TMP/(name+'.ttf')
 if not p.exists():instantiateVariableFont(Font(ROOT/'assets/fonts/Inter.ttf'),{'opsz':14,'wght':weight},inplace=False).save(p)
 pdfmetrics.registerFont(TTFont(name,str(p)))
BG='#0B0C0F';PANEL='#191B20';TEXT='#F3F2EF';MUTED='#A7A9B1';ACC='#FF8A36';LINE='#D5D7DD';EDGE='#34363D'
W,H=1190.55,841.89
def txt(x,y,s,size=9,bold=False,col=TEXT):
 c.setFillColor(HexColor(col));c.setFont('InterSB' if bold else 'Inter',size);c.drawString(x,y,s)
def line(points,col=LINE,width=1.35,dash=False):
 c.setStrokeColor(HexColor(col));c.setLineWidth(width);c.setDash([3,3] if dash else [])
 p=c.beginPath();p.moveTo(*points[0])
 for xy in points[1:]:p.lineTo(*xy)
 c.drawPath(p);c.setDash([])
def box(x,y,w,h,fill=PANEL,stroke=None):
 c.setFillColor(HexColor(fill));c.setStrokeColor(HexColor(stroke or fill));c.rect(x,y,w,h,fill=1,stroke=bool(stroke))
def dot(x,y,col=LINE):c.setFillColor(HexColor(col));c.circle(x,y,2.4,fill=1,stroke=0)
def ground(x,y):
 line([(x,y),(x,y-5)])
 for i,w in enumerate([15,10,5]):line([(x-w/2,y-5-i*3),(x+w/2,y-5-i*3)])
def tag(x,y,s,col=ACC):
 w=pdfmetrics.stringWidth(s,'InterSB',8)+12;box(x,y-4,w,15,col);txt(x+6,y,s,8,True,BG);return w
def resistor(x,y,label):
 line([(x,y),(x+8,y)]);box(x+8,y-4,30,8,PANEL,LINE);line([(x+38,y),(x+46,y)]);txt(x-1,y+12,label,8,True)
def cap(x,y,label):
    line([(x,y+15),(x,y+3)]);line([(x-8,y+3),(x+8,y+3)]);line([(x-8,y-3),(x+8,y-3)]);line([(x,y-3),(x,y-15)]);txt(x+12,y-3,label,8)
    if 'uF' in label:txt(x-15,y+6,'+',8,True,ACC)
def led(x,y,label):
 line([(x,y),(x+9,y)]);line([(x+9,y-6),(x+9,y+6),(x+23,y),(x+9,y-6)]);line([(x+25,y-7),(x+25,y+7)]);line([(x+25,y),(x+43,y)])
 for k in [0,7]:line([(x+18+k,y+9),(x+24+k,y+15),(x+20+k,y+14)] )
 txt(x+5,y-18,'A',7,bold=True);txt(x+24,y-18,'K',7,bold=True);txt(x+53,y+12,label,9,True)
def panel(x,y,w,h,title):
 box(x,y,w,h);line([(x,y+h),(x+38,y+h)],ACC,3);txt(x+14,y+h-23,title,10,True,ACC)
def para(x,top,w,s,size=9,leading=12):
 p=Paragraph(s,ParagraphStyle('p',fontName='Inter',fontSize=size,leading=leading,textColor=HexColor(TEXT)))
 _,h=p.wrap(w,1000);p.drawOn(c,x,top-h);return top-h
def pinbox(x,y,title,rows):
 txt(x,y,title,9,True,ACC)
 for i,s in enumerate(rows):
  yy=y-20-i*19;box(x,yy-4,15,15,BG);txt(x+4,yy,str(i+1),8,True);txt(x+22,yy,s,8.5,False,ACC if ('TX' in s or 'RX' in s) else TEXT)
def board(full):
 panel(28,290,320,458,'01 / E32R28T CONNECTIONS')
 txt(42,708,'REAR VIEW',9,True);txt(42,694,'Antenna up / USB-C down',8,col=MUTED)
 s=145/1145;ox=40-690*s;oy=418-310*s
 board_image=HERE/'board-reference.png' if (HERE/'board-reference.png').exists() else TMP/'board-000.png'
 c.saveState();p=c.beginPath();p.rect(40,418,145,251);c.clipPath(p,stroke=0);c.drawImage(str(board_image),ox,oy,2560*s,2560*s,mask='auto');c.restoreState()
 for label,x,y,w,h in [('A',41,600,19,32),('B',41,554,19,32),('C',163,476,18,27)]:
  c.setStrokeColor(HexColor(ACC));c.setLineWidth(1.6);c.rect(x,y,w,h,fill=0,stroke=1);tag(x-2,y+h+5,label)
 pinbox(202,672,'A / P4', ['3V3','IO35 / RX','NC - leave open','GND'])
 pinbox(202,560,'B / P3', ['IO23 / MOSI','IO19 / MISO','IO18 / SCK','IO27 / TX'])
 txt(202,447,'C / BATTERY',9,True,ACC);txt(202,427,'Top: BAT +',9);txt(202,409,'Bottom: BAT -',9)
 y=391
 for s in ['Pin order is top to bottom in this rear view, NOT the mating plug view. Verify silkscreen and polarity.',
           'Use matching 1.25 mm connector pigtails. Ordinary 2.54 mm Dupont plugs do not fit these sockets.',
           'GPIO34 is already wired to the onboard 100k / 100k battery divider. Do not connect BAT+ directly to it.']:
  y=para(42,y,291,s,8.6,11.5)-8
def full_power():
 panel(370,595,792,153,'02 / BATTERY POWER - TWO ISOLATED SWITCH POLES')
 box(389,655,122,54,BG,EDGE);txt(400,687,'PROTECTED LiPo',9,True);txt(400,670,'1S / 3.7 V / 3000 mAh',8)
 txt(505,697,'+',9,True,ACC);txt(505,659,'-',9,True)
 line([(511,696),(531,696)],ACC);box(531,692,23,8,PANEL,ACC);line([(554,696),(578,696)],ACC);txt(529,711,'F1 / 1 A',8,True)
 line([(578,696),(578,649),(610,649)],ACC);line([(578,696),(610,696)],ACC);dot(578,696,ACC)
 for y in [696,649]:
  dot(610,y,ACC);dot(647,y,ACC);line([(610,y),(639,y+12)],ACC,1.8);line([(647,y),(736,y)],ACC)
 line([(630,713),(630,633)],MUTED,.8,True);txt(670,711,'SW1 / DPST / 2 A DC',8,True)
 tag(738,693,'C / BAT+');txt(834,693,'Board power + charger',9)
 tag(738,663,'VIR');txt(785,663,'LED supply',9)
 line([(736,649),(935,649),(1084,649)],ACC);dot(935,649,ACC);dot(1084,649,ACC)
 line([(935,649),(935,647)]);line([(1084,649),(1084,647)]);cap(935,632,'470 uF / 10 V');cap(1084,632,'100 nF')
 ground(935,617);ground(1084,617)
 line([(511,664),(530,664),(530,644)]);ground(530,644);txt(389,624,'BAT- is the common ground.',8,col=MUTED)
 txt(384,603,'SW1 OFF isolates the battery and LEDs; USB can still power the screen. SW1 ON is required to charge.',8,col=MUTED)
def full_tx():
 panel(370,372,490,205,'03 / TRANSMIT - TWO SPECIFIED 940 nm EMITTERS')
 tw=tag(384,523,'VIR');line([(384+tw,526),(440,526),(440,480)],ACC);dot(440,526,ACC)
 for n,y in [(1,526),(2,480)]:
  line([(440,y),(465,y)],ACC);resistor(465,y,f'R{n} / 33 ohm');line([(511,y),(555,y)]);led(555,y,f'TSAL6200 / {n}');line([(598,y),(818,y)]);dot(818,y)
 line([(818,526),(818,453)]);txt(823,455,'D / 3',7.8)
 # N-MOSFET: G=1, S=2, D=3; electrical symbol, not package geometry.
 line([(797,422),(797,446)]);line([(806,422),(806,446)]);line([(806,446),(818,446),(818,453)]);line([(806,422),(818,422),(818,401)])
 txt(803,469,'Q1',8,True);txt(768,393,'AO3400A',8,True);txt(822,409,'S / 2',7.8)
 tw=tag(384,431,'TX');line([(384+tw,434),(425,434)]);resistor(425,434,'Rg / 100 ohm');line([(471,434),(493,434)])
 line([(493,426),(493,442)]);line([(499,426),(499,442)]);line([(499,434),(797,434)]);txt(484,452,'Cg / 10 nF',8,True)
 txt(768,444,'G / 1',7.8)
 # Pulldown and negative Schottky clamp.
 dot(626,434);line([(626,434),(626,422)]);box(622,398,8,24,PANEL,LINE);line([(626,398),(626,389)]);ground(626,389);txt(559,406,'Rpd 10k',8)
 dot(716,434);line([(716,434),(716,420)]);line([(708,420),(724,420)]);line([(709,407),(723,407),(716,419),(709,407)]);line([(716,407),(716,389)]);ground(716,389)
 txt(729,415,'K to gate',7.5);txt(729,403,'A to GND',7.5);txt(665,394,'BAT54',8,True)
 ground(818,401)
def receive(full):
 panel(878,372,284,205,'04 / RECEIVE - 3.3 V LOGIC')
 tw=tag(892,523,'3V3')
 if full:
  line([(892+tw,526),(942,526)]);resistor(942,526,'Rf / 100 ohm');line([(988,526),(1131,526),(1131,512)])
  for x in [996,1040]:
   dot(x,526);line([(x,526),(x,524)]);line([(x-7,520),(x+7,520)]);line([(x,524),(x,520)]);line([(x-7,514),(x+7,514)]);line([(x,514),(x,499)]);ground(x,499)
  txt(978,480,'10 uF',8);txt(1021,480,'100 nF',8);txt(982,516,'+',8,True,ACC)
 else:line([(892+tw,526),(1131,526),(1131,512)])
 box(1068,424,79,88,BG,EDGE);txt(1075,491,'VS1838B',9,True);txt(1075,476,'VCC',8);txt(1075,451,'OUT',8);txt(1075,432,'GND',8)
 txt(1135,515,'VCC',7,col=MUTED)
 tw=tag(994,451,'RX');line([(994+tw,454),(1068,454)]);line([(1109,424),(1109,414)]);ground(1109,414)
 txt(892,391,'Logical pins, NOT physical leg order.',8,col=MUTED);txt(892,380,'Check receiver markings / datasheet.',8,col=MUTED)
def simple_power():
 panel(370,595,792,153,'02 / BATTERY POWER - ONBOARD USB-C CHARGING')
 box(391,649,179,58,BG,EDGE);txt(405,682,'PROTECTED 1S LiPo',12,True);txt(405,663,'3.7 V / about 3000 mAh',10)
 line([(570,692),(626,692)],ACC);dot(626,692,ACC);line([(626,692),(663,706)],ACC,1.8);dot(671,692,ACC);line([(671,692),(763,692)],ACC);tag(766,689,'C / BAT+');txt(681,710,'Optional SPST switch',9)
 line([(570,659),(763,659)]);tag(766,656,'C / BAT-');ground(708,659)
 txt(888,690,'USB-C',12,True);txt(888,669,'Program + recharge onboard',10)
 txt(391,618,'Opening the battery switch stops battery charging. It does not turn the board off while USB is connected.',9,col=MUTED)
def simple_tx():
 panel(370,372,490,205,'03 / TRANSMIT - ONE LED, NO MOSFET')
 tw=tag(391,505,'TX');line([(391+tw,508),(468,508)]);resistor(468,508,'R1 / 150 ohm / 1/4 W');line([(514,508),(584,508)]);led(584,508,'940 nm LED');line([(627,508),(814,508),(814,476)]);ground(814,476)
 txt(391,454,'About 12-15 mA while GPIO27 is HIGH.',12,True)
 para(391,439,439,'Use a standard 20 mA-rated emitter, or a TSAL6200. One LED concentrates the available GPIO drive in a single emitter; it does not increase the ESP32 current limit.',9.5,13)
 txt(391,384,'No battery feed to this LED. Never fit the full build\'s 33 ohm resistor here.',8.5,col=ACC)
def guard(full):
 panel(370,290,792,64,'05 / FIRMWARE PROFILE')
 txt(384,314,'FULL  /  nominal 33% carrier  /  GPIO34 battery sensing  /  TX blocked below ~3.40 V; resumes at ~3.55 V.' if full else 'SIMPLE  /  one directly driven LED  /  nominal 50% carrier  /  no battery-only transmit lockout.',9.3,True)
 txt(384,300,'Flash the full profile. Calibrate the onboard battery reading against a meter. USB can mask an absent battery.' if full else 'Flash the simple profile. Both builds support touch, IR learning, Favorites and eight saved-code slots.',8.5,col=MUTED)
def bottom(full):
 panel(28,56,545,211,'06 / PARTS CHECKLIST')
 common=[('1 each','E32R28T; USB-C data cable; protected 1S LiPo + matching plug')]
 rows=common+([('2','Vishay TSAL6200 emitters - do not substitute uncharacterized kit LEDs'),('1','AO3400A + SOT-23 breakout: G=1, S=2, D=3'),('2','33 ohm, 1%, 1/2 W resistors (one per emitter)'),('1 each','Gate: 100 ohm, 10 nF C0G, 10k ohm, BAT54 + adapter'),('1 each','Supply: 470 uF / 10 V electrolytic + 100 nF ceramic'),('1 + filter','VS1838B/HX1838B; 100 ohm + 10 uF (>=6.3 V) + 100 nF'),('1 each','DPST switch >=2 A DC; 1 A inline fuse + holder'),('as needed','1.25 mm pigtails, perfboard, insulated wire, enclosure and mounts')] if full else [('1 + 1','940 nm, 20 mA-rated emitter + VS1838B/HX1838B receiver'),('1','150 ohm resistor, 1/4 W'),('none','No MOSFET, gate network or bulk capacitor required'),('optional','100 nF across receiver VCC/GND if learning is noisy'),('optional','SPST battery switch with adequate DC rating'),('as needed','1.25 mm pigtails, perfboard, insulated wire, enclosure and mounts')])
 for i,(n,s) in enumerate(rows):y=226-i*17.5;txt(42,y,n,8.7,True,ACC);txt(108,y,s,8.6)
 panel(594,56,568,211,'07 / ASSEMBLY + OPERATING NOTES')
 notes=(['Use one 33 ohm resistor per TSAL6200: approximately 60-90 mA per LED while on. Aim emitters forward for reach; angling them outward widens coverage. Test range with your equipment.',
 'BAT54 cathode K goes to gate; anode A to GND. RC time constant is about 0.1 ms; near-zero gate voltage takes about 0.5 ms. This does not protect against stuck PWM or a failed MOSFET.',
 'Keep LED power loops short; return LED current to battery ground, not through GPIO wiring. Place capacitors at their loads; observe electrolytic polarity.',
 'Charge with SW1 ON and IR idle. Disconnect USB for high-power transmission. The 3.4 V software guard is not battery protection or USB detection.',
 'Optional four LEDs: add two identical 33 ohm branches; use 1000 uF / 10 V bulk and recheck wiring, current and temperature.',
 'Start with 150 ohm LED resistors. Verify current and gate waveform, then fit 33 ohm and recheck. Insulate splices; do not solder to a LiPo pouch. Disconnect all power before wiring.'] if full else ['LED anode A is the long lead; cathode K is the flat-side lead. Connect GPIO27 -> 150 ohm -> A, then K -> GND. Always use the series resistor.',
 'Receiver output must be 3.3 V logic. Keep its wires short; a 100 nF bypass capacitor is the first improvement if capture is noisy.',
 'GPIO27 is separate from display and resistive-touch buses on this E32R28T. GPIO35 is input-only. Leave unused connector pins open.',
 'Favorites and up to eight saved codes use onboard flash; no SD card is needed. Learning pauses during transmission. The ~38 kHz receiver captures IR timing, not carrier frequency or RF/Bluetooth signals.',
 'Battery connector polarity and receiver lead order must be verified. Power off and unplug USB before wiring. The first touch after backlight sleep wakes without transmitting.'])
 y=235
 for s in notes:y=para(608,y,539,s,8.5,11)-6
 assert y>=63,(full,y)
def footer():
 txt(28,43,'COMMON SOFTWARE / Touch UI, Favorites, eight saved codes; RX paused during TX; flash storage, no SD. ~38 kHz IR receiver, not RF/Bluetooth.',7.5,col=MUTED)
 x=28
 for label,url in [('BOARD / PINOUT','https://www.lcdwiki.com/2.8inch_ESP32-32E_Display'),('SCHEMATIC','https://www.lcdwiki.com/res/E32R28T/2.8inch_ESP32-32_Display_Schematic.pdf'),('LED DATASHEET','https://www.vishay.com/docs/81010/tsal6200.pdf'),('MOSFET DATASHEET','https://www.aosmd.com/sites/default/files/res/datasheets/AO3400A.pdf'),('IR SOFTWARE','https://github.com/crankyoldgit/IRremoteESP8266')]:
  txt(x,29,label,7.5,True,ACC);w=pdfmetrics.stringWidth(label,'InterSB',7.5);c.linkURL(url,(x,26,x+w,39),relative=0);x+=w+27
 txt(850,29,'POWER OFF BEFORE WIRING  /  VERIFY ON HARDWARE',8,True,MUTED)
for full in [True,False]:
 path=OUT/('ESP32_CYD_IR_Remote_Full_Build.pdf' if full else 'ESP32_CYD_IR_Remote_Simple_Build.pdf')
 c=canvas.Canvas(str(path),pagesize=(W,H));c.setTitle('PULSE / '+('Full' if full else 'Simple')+' IR remote assembly sheet');box(0,0,W,H,BG)
 line([(28,817),(1162,817)],ACC,3);txt(28,784,'PULSE / '+('FULL' if full else 'SIMPLE')+' IR REMOTE',26,True)
 txt(28,763,'BATTERY-POWERED DRIVER / TWO EMITTERS / DC-BLOCKED GATE' if full else 'DIRECT GPIO DRIVE / ONE EMITTER / MINIMAL COMPONENTS',10,col=MUTED)
 txt(1018,786,'E32R28T',17,True,ACC);txt(1018,766,'ASSEMBLY SHEET',9,col=MUTED)
 board(full);(full_power if full else simple_power)();(full_tx if full else simple_tx)();receive(full);guard(full);bottom(full);footer();c.showPage();c.save();print(path)
