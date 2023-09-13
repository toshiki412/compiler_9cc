# ubuntu���ō��
FROM ubuntu:23.04

RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y gcc g++ make git binutils libc6-dev gdb sudo

# g++�̃o�[�W������؂�ւ�
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 100

# user�Ƃ������O�Ń��[�U��ǉ�����
RUN adduser --disabled-password --gecos '' user

# user��sudo������t����
RUN echo 'user ALL=(root) NONPASSED:ALL' > /etc/sudoers.d/user

# �f�t�H���g�̃��[�U��user�ɂ���
USER user

WORKDIR /home/user