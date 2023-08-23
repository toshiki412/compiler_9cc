# ubuntu環境で作る
FROM ubuntu:latest

RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y gcc g++ make git binutils libc6-dev gdb sudo

# userという名前でユーザを追加する
RUN adduser --disabled-password --gecos '' user

# userにsudo権限を付ける
RUN echo 'user ALL=(root) NONPASSED:ALL' > /etc/sudoers.d/user

# デフォルトのユーザをuserにする
USER user

WORKDIR /home/user