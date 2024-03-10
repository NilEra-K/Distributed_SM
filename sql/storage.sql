DROP DATABASE IF EXISTS tnv_storagedb;
CREATE DATABASE tnv_storagedb;
USE tnv_storagedb;

CREATE TABLE `t_file_01` (
    `id` VARCHAR(256) NOT NULL DEFAULT '',
    `appid` VARCHAR(32) DEFAULT NULL,
    `userid` VARCHAR(128) DEFAULT NULL,
    `status` TINYINT(4) DEFAULT NULL,
    `file_path` VARCHAR(512) DEFAULT NULL,
    `file_size` BIGINT(20) DEFAULT NULL,
    `create_time` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `t_file_02` (
    `id` VARCHAR(256) NOT NULL DEFAULT '',
    `appid` VARCHAR(32) DEFAULT NULL,
    `userid` VARCHAR(128) DEFAULT NULL,
    `status` TINYINT(4) DEFAULT NULL,
    `file_path` VARCHAR(512) DEFAULT NULL,
    `file_size` BIGINT(20) DEFAULT NULL,
    `create_time` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE `t_file_03` (
    `id` VARCHAR(256) NOT NULL DEFAULT '',
    `appid` VARCHAR(32) DEFAULT NULL,
    `userid` VARCHAR(128) DEFAULT NULL,
    `status` TINYINT(4) DEFAULT NULL,
    `file_path` VARCHAR(512) DEFAULT NULL,
    `file_size` BIGINT(20) DEFAULT NULL,
    `create_time` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    `update_time` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY(`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
