git submodule foreach git pull origin main

mv ./software/src/main.c main.c

rm ./software/inc -rf
rm ./software/lib -rf
rm ./software/lnk -rf
rm ./software/src -rf
rm ./software/targets -rf

cp ./STM32F412/inc ./software/ -r
cp ./STM32F412/lib ./software/ -r
cp ./STM32F412/lnk ./software/ -r
cp ./STM32F412/src ./software/ -r
cp ./STM32F412/targets ./software/ -r

rm ./software/src/main.c
mv main.c ./software/src

