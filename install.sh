PIO_PATH =/home/polanco/.platformio/penv/bin/pio

echo "Instalando dependencias"
sudo apt install make build-essential python python-pip gcc-avr avr-libc avrdude
echo "Instalando dependencias de python"
sudo pip install pySerial numpy matplotlib argparse
echo "Creando carpetas necesarias"
sudo mkdir ./obj/
sudo mkdir ./bin/
echo "Includendo platformio al PATH"
sudo ls -s $PIO_PATH /usr/bin/