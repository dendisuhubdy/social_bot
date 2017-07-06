socialbot
=====================

## What is it?
It's a simple C++ program to translate github commits into social networks posts: Twitter, Telegram ...

![demo1](docs/pics/01.png)

## Example

You can find results of working of this bot in Twitter with hashtag **#riscv_vhdl**.  
Or you can join to Telegram's public channel **riscv_vhdl**.

## How to setup

This program will be cross-platform soon, but for now MS Visual Studio is used to open solution and build 
*Release* target. This repository already includes necessary libraries (libcurl.lib).

## Edit JSON-configuration file

Everytime *socialbot.exe* must be started with one argument - configuration
file containing information about your accounts. Edit *configs/example.json* file
using the following tips:

  - Generate github token and write the following field:  
    **Config["github"]["author"]**. - github account login  
    **Config["github"]["repo_name"]**. - github repository name to track commits  
    **Config["github"]["oauth_token"]** - github application token.
  - Generate Twitter OAuth keys and token and edit the following fields:  
     **Config["twitter"]["username"],** - Twitter account login  
     **Config["twitter"]["password"],** - Twitter account password  
     **Config["twitter"]["oauth_consumer_key"],** - Twitter generated data for developers  
     **Config["twitter"]["consumer_secret"],** - Twitter generated data for developers  
     **Config["twitter"]["oauth_token"],** - Twitter generated data for developers  
     **Config["twitter"]["token_secret"]** - Twitter generated data for developers.
  - Create Telegram bot and edit the following fields:  
     **Config["telegram"]["token"]** - Telegram's bot token ID assigning on the bot creation.  
     **Config["telegram"]["chat_id"]** - Your Telegram's channel ID

## Start bot

I'm using Windows "Task Scheduler" to run this bot with certain intervals.

That's all.



