# Brogue v 1.7.2 汉化版

## 冒险者，欢迎来到厄运之地牢

Brogue 可能是近些年 Roguelike 新作中最受欢迎的一个。这里是基于 [v 1.7.2 clight](http://brogue.createforumhosting.com/brogue-tiles-with-continuous-lighting-updated-4-june-2013-t857.html) 的汉化版。在熟悉的语言环境下开始冒险吧！

## 下载

本页面上方 Releases 里有带有打包好的游戏下载。

## 目前状况

基本汉化完毕，应该可以正常通关。中断和录像功能基本正常。

## 编译

由于 Brogue 本身使用了很多 C99 的功能，Windows 下几乎没有办法用 VS 编译。目前仅支持在 Windows 下使用 MinGW。(推荐 [TDM-GCC](http://tdm-gcc.tdragon.net/))  
checkout build 分支，确定 make, gcc, windres 在你的 Path 上。

    make -f Makefile.windows

Release 版本

    make -f Makefile.windows RELEASE=TRUE

## TODO

* mac builds
* update with 1.7.3 