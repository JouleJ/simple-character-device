# Homework assignment 1

## Setup

```
make
sudo insmod phone_book.ko
sudo dmesg | grep phone
sudo mknod /dev/phone_book c MAJOR_NUM 0
sudo chmod a+rw /dev/phone_book
```
where `MAJOR_NUM` can be obtained from output of `dmesg`

## Example Usage

```
echo 'insert john smith 42 +1234567890 john.smith@gmail.com' > /dev/phone_book
echo 'insert jane doe 19 +0987654321 jane.doe@yahoo.com' > /dev/phone_book
echo 'insert ivan petrov 33 +57-77-55 pivan@mail.ru' > /dev/phone_book
echo 'get smith' > /dev/phone_book && cat /dev/phone_book # will output smith's data
echo 'get petrov' > /dev/phone_book && cat /dev/phone_book # will output petrov's data
echo 'get doe' > /dev/phone_book && cat /dev/phone_book # will output doe's data
echo 'remove doe' > /dev/phone_book
echo 'get doe' > /dev/phone_book && cat /dev/phone_book # will no longer output doe's data
```
