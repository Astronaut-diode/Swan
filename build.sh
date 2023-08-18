#!/bin/bash
mysql -u root -p [输入密码] Swan << EOF 2>/dev/null

create database if not exists Swan character set utf8mb4 collate utf8mb4_unicode_ci;
use Swan;
drop table if exists user;
create table if not exists user(userId   int primary key auto_increment comment '用户id',username varchar(128) comment '用户账户',password varchar(128) comment '用户密码') comment '用户表' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists group;
create table if not exists group(groupId   int primary key auto_increment comment '群的id',groupName varchar(128) comment '群组的名字',masterId  int comment '群主的用户id') comment '群组表' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists friendRelation;
create table if not exists friendRelation(friendRelationId int primary key auto_increment comment '一组关系的主键',sourceId         int comment '出发点（好友id）',destId           int comment '目标点',lastReadTime     timestamp comment '最后一次dest_id用户接收source_id(仅仅是用户)信息的时间。') comment '关系表，仅记录好友关系，以及该用户接收该关系发送过来信息的最后一次时间。' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists groupRelation;
create table if not exists groupRelation(groupRelationId int primary key auto_increment comment '一组关系的主键',sourceId        int comment '出发点（群组id）',destId          int comment '目标点',lastReadTime    timestamp comment '最后一次dest_id用户接收source_id(仅仅是群组)信息的时间。') comment '关系表，仅记录群组关系以及当前用户接收该群消息的最后一次时间。' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists friendMessage;
create table if not exists friendMessage(friendMessageId int primary key auto_increment comment '用户聊天记录的id',sourceId        int comment '发送信息的人',destId          int comment '接收信息的人',content         varchar(1024) comment '发送的信息内容，长度最长是1024bytes',sendTime        timestamp comment '信息的发送时间，用于让目标用户找到未读信息') comment '用户聊天记录表' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists groupMessage;
create table if not exists groupMessage(groupMessageId int primary key auto_increment comment '群组聊天记录的id',sourceId       int comment '发送信息的群组',innerSourceId  int comment '发送信息的具体用户的id',content        varchar(1024) comment '发送的信息内容，长度最长是1024bytes',sendTime       timestamp comment '信息的发送时间，用于找到未读信息') comment '群组聊天记录表。' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists friendRequest;
create table if not exists friendRequest(friendRequestId int primary key auto_increment comment '请求的id',sourceId        int comment '发送信息的人的id',destId          int comment '接收信息的人的id',processed       bool comment '请求是否已经被处理了。') comment '好友请求表' character set utf8mb4 collate utf8mb4_unicode_ci;
drop table if exists groupRequest;
create table if not exists groupRequest(groupRequestId int primary key auto_increment comment '请求的id',sourceId       int comment '发送信息的人的id',destId         int comment '接收信息的人的id',processed      bool comment '请求是否已经被处理了。') comment '群组请求表' character set utf8mb4 collate utf8mb4_unicode_ci;
exit
EOF