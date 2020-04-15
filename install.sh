echo "Instalando dependencias"
sudo apt install make build-essential python python-pip gcc-avr avr-libc avrdude
echo "Instalando dependencias de python"
sudo pip install pySerial numpy matplotlib argparse
echo "Includendo platformio al PATH"
sudo ls -s /home/leonelmora/.platformio/penv/bin/pio /usr/bin/
echo "Creando carpetas necesarias"
sudo mkdir ./obj/
sudo mkdir ./bin/