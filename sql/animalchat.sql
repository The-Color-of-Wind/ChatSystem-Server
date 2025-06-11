/*
SQLyog Ultimate v12.09 (64 bit)
MySQL - 8.0.42-0ubuntu0.20.04.1 : Database - animalchat
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
CREATE DATABASE /*!32312 IF NOT EXISTS*/`animalchat` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */;

USE `animalchat`;

/*Table structure for table `m_chatlist` */

DROP TABLE IF EXISTS `m_chatlist`;

CREATE TABLE `m_chatlist` (
  `user_id` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `comm_id` varchar(20) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `chat_type` enum('friend','group') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `last_message` text,
  `last_message_type` enum('text','emoticon','image','audio','vedio','file') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL,
  `last_time` datetime NOT NULL,
  `unread_count` int NOT NULL DEFAULT '0',
  `chat_status` enum('chatting','close') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  PRIMARY KEY (`user_id`,`comm_id`),
  CONSTRAINT `m_chatlist_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `m_user` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/*Table structure for table `m_friends_message` */

DROP TABLE IF EXISTS `m_friends_message`;

CREATE TABLE `m_friends_message` (
  `message_id` bigint NOT NULL AUTO_INCREMENT,
  `sender_id` varchar(20) NOT NULL,
  `receiver_id` varchar(20) NOT NULL,
  `chat_type` enum('friend','group') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `message_body` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci,
  `message_type` enum('text','emoticon','image','audio','video','file') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `file_path` varchar(255) DEFAULT NULL,
  `message_status` enum('received','read','delete','send') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `message_timestamp` datetime NOT NULL,
  PRIMARY KEY (`message_id`),
  KEY `sender_id` (`sender_id`),
  KEY `receiver_id` (`receiver_id`),
  CONSTRAINT `m_friends_message_ibfk_4` FOREIGN KEY (`sender_id`) REFERENCES `m_user` (`user_id`),
  CONSTRAINT `m_friends_message_ibfk_5` FOREIGN KEY (`receiver_id`) REFERENCES `m_user` (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=231132 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/*Table structure for table `m_friends_relation` */

DROP TABLE IF EXISTS `m_friends_relation`;

CREATE TABLE `m_friends_relation` (
  `user_id` varchar(20) NOT NULL,
  `friend_id` varchar(20) NOT NULL,
  `friend_relation` enum('pending','accepted','refused') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT 'pending',
  `created_relation` datetime NOT NULL,
  PRIMARY KEY (`user_id`,`friend_id`),
  KEY `friend_id` (`friend_id`),
  CONSTRAINT `m_friends_relation_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `m_user` (`user_id`),
  CONSTRAINT `m_friends_relation_ibfk_2` FOREIGN KEY (`friend_id`) REFERENCES `m_user` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/*Table structure for table `m_groups` */

DROP TABLE IF EXISTS `m_groups`;

CREATE TABLE `m_groups` (
  `group_id` varchar(20) NOT NULL,
  PRIMARY KEY (`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/*Table structure for table `m_groups_relation` */

DROP TABLE IF EXISTS `m_groups_relation`;

CREATE TABLE `m_groups_relation` (
  `group_id` varchar(20) NOT NULL,
  `user_id` varchar(20) NOT NULL,
  PRIMARY KEY (`group_id`,`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/*Table structure for table `m_media_file` */

DROP TABLE IF EXISTS `m_media_file`;

CREATE TABLE `m_media_file` (
  `file_id` bigint NOT NULL AUTO_INCREMENT,
  `message_id` bigint NOT NULL,
  `file_type` enum('text','image','file','video') NOT NULL,
  `file_url` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `created_at` datetime NOT NULL,
  PRIMARY KEY (`file_id`),
  KEY `message_id` (`message_id`),
  CONSTRAINT `m_media_file_ibfk_1` FOREIGN KEY (`message_id`) REFERENCES `m_friends_message` (`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/*Table structure for table `m_user` */

DROP TABLE IF EXISTS `m_user`;

CREATE TABLE `m_user` (
  `user_id` varchar(20) NOT NULL,
  `user_password` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `user_phone` varchar(12) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `user_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `user_email` varchar(30) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT 'aa@an.com',
  `user_avatar` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT '/default.jpg',
  `user_status` enum('active','inactive','busy') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT 'inactive',
  `user_created` datetime NOT NULL,
  PRIMARY KEY (`user_id`),
  UNIQUE KEY `user_email` (`user_email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

/* Procedure structure for procedure `AddFriend` */

/*!50003 DROP PROCEDURE IF EXISTS  `AddFriend` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`%` PROCEDURE `AddFriend`(
	IN a_user_id varchar(20),
	IN a_friend_id VARCHAR(20)
    )
BEGIN
	INSERT INTO m_friends_relation (user_id, friend_id, friend_relation, created_relation)
	VALUES (a_user_id, a_friend_id, 'pending', NOW());
	
	INSERT INTO m_friends_relation (user_id, friend_id, friend_relation, created_relation)
        VALUES (a_friend_id, a_user_id, 'pending', NOW());
    
    END */$$
DELIMITER ;

/* Procedure structure for procedure `AddFriendMessage` */

/*!50003 DROP PROCEDURE IF EXISTS  `AddFriendMessage` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`%` PROCEDURE `AddFriendMessage`(
	in a_sender_id varchar(20),
	in a_receiver_id varchar(20),
	IN a_chat_type ENUM('friend', 'group'),	
	in a_message_body text,
	IN a_message_type ENUM('text', 'image', 'audio', 'vedio', 'file'),
	in a_file_path VARCHAR(255), 
	IN a_message_status ENUM('send', 'received', 'read', 'delete'),
	in a_message_timestamp datetime,
	OUT new_message_id VARCHAR(255)  -- 修改为 VARCHAR 类型输出参数
    )
BEGIN
	DECLARE chat_exists INT;
	DECLARE a_chatlist_message_body TEXT;
    /*先插入m_friends_message*/
	insert into m_friends_message(sender_id, receiver_id, chat_type, message_body, message_type, file_path, message_status,  message_timestamp)
	values (a_sender_id, a_receiver_id, a_chat_type, a_message_body, a_message_type, a_file_path, a_message_status, a_message_timestamp);
	SET new_message_id = CAST(LAST_INSERT_ID() AS CHAR(255));
	
    /*判断消息列表是否存在*/
	SELECT COUNT(*) INTO chat_exists
	FROM m_chatlist
	WHERE (m_chatlist.user_id = a_sender_id AND m_chatlist.comm_id = a_receiver_id);
	
     /*判断消息类型*/
	IF a_message_type = 'text' THEN
		SET a_chatlist_message_body = a_message_body;
	ELSEIF a_message_type = 'emoticon' THEN
		SET a_chatlist_message_body = '[动画表情]';
	ELSEIF a_message_type = 'image' THEN
		SET a_chatlist_message_body = '[图片]';
	ELSEIF a_message_type = 'audio' THEN
		SET a_chatlist_message_body = '[语音]';
	ELSEIF a_message_type = 'video' THEN
		SET a_chatlist_message_body = '[视频]';
	ELSEIF a_message_type = 'file' THEN
		SET a_chatlist_message_body = '[文件]';
	END IF;
	
     /*更新m_chatlist*/
	/*IF chat_exists > 0 THEN 
		update m_chatlist
		set last_message_type = a_message_type, last_message = a_chatlist_message_body, last_time = a_message_timestamp, chat_status = '1'
		where (m_chatlist.user_id = a_sender_id AND m_chatlist.comm_id = a_receiver_id);
		UPDATE m_chatlist
		SET last_message_type = a_message_type, last_message = a_chatlist_message_body, last_time = a_message_timestamp, unread_count = unread_count+1, chat_status = '1'
		WHERE (m_chatlist.comm_id = a_sender_id AND m_chatlist.user_id = a_receiver_id);
	else
		insert into m_chatlist(user_id, comm_id, chat_type, last_message, last_message_type, last_time, unread_count)
		VALUES(a_sender_id, a_receiver_id, a_chat_type, a_chatlist_message_body, a_message_type, a_message_timestamp, '1');
		INSERT INTO m_chatlist(user_id, comm_id, chat_type, last_message, last_message_type, last_time, unread_count)
		VALUES(a_receiver_id, a_sender_id, a_chat_type, a_chatlist_message_body, a_message_type, a_message_timestamp, '1');
	end if;*/
	
	update m_chatlist
	set last_message_type = a_message_type, last_message = a_chatlist_message_body, last_time = a_message_timestamp, unread_count = 0, chat_status = '1'
	where (m_chatlist.user_id = a_sender_id AND m_chatlist.comm_id = a_receiver_id);
	UPDATE m_chatlist
	SET last_message_type = a_message_type, last_message = a_chatlist_message_body, last_time = a_message_timestamp, unread_count = unread_count+1, chat_status = '1'
	WHERE (m_chatlist.comm_id = a_sender_id AND m_chatlist.user_id = a_receiver_id);
	
    END */$$
DELIMITER ;

/* Procedure structure for procedure `DeleteFriend` */

/*!50003 DROP PROCEDURE IF EXISTS  `DeleteFriend` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`%` PROCEDURE `DeleteFriend`(
	IN d_user_id VARCHAR(20),
	IN d_friend_id VARCHAR(20)
    )
BEGIN
	DELETE FROM m_friends_relation
	WHERE (user_id = d_user_id AND friend_id = d_friend_id) or (user_id = d_friend_id AND friend_id = d_user_id);
	
	DELETE FROM m_chatlist
	WHERE (user_id = d_user_id AND comm_id = d_friend_id) or (user_id = d_friend_id AND comm_id = d_user_id);
	
	update m_friends_message
	set message_status = 'delete'
	where (sender_id = d_user_id AND receiver_id = d_friend_id) or (sender_id = d_friend_id AND receiver_id = d_user_id);
    END */$$
DELIMITER ;

/* Procedure structure for procedure `UpdateFriend` */

/*!50003 DROP PROCEDURE IF EXISTS  `UpdateFriend` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`%` PROCEDURE `UpdateFriend`(
	IN u_user_id VARCHAR(20),
	IN u_friend_id VARCHAR(20),
	IN u_friend_relation ENUM('accepted', 'refused')
    )
BEGIN
	if u_friend_relation = 'accepted' then
		UPDATE m_friends_relation
		SET friend_relation = u_friend_relation
		WHERE (user_id = u_friend_id AND friend_id = u_user_id) OR (user_id = u_user_id AND friend_id = u_friend_id);
		        
		INSERT INTO m_chatlist(user_id, comm_id, chat_type, last_message, last_message_type, last_time, unread_count, chat_status)
		VALUES(u_user_id, u_friend_id, 'friend', '', 'text', NOW(), '0', 'close');
	
		INSERT INTO m_chatlist(user_id, comm_id, chat_type, last_message, last_message_type, last_time, unread_count, chat_status)
		VALUES(u_friend_id, u_user_id, 'friend', '', 'text', NOW(), '0', 'close');
		
		CALL AddFriendMessage(u_user_id, u_friend_id, 'friend', '我们已经是好友了，可以开始聊天了！', 'text', '', 'send', NOW(), @new_message_id);
	
	elseif u_friend_relation = 'refused' then
		delete from m_friends_relation
		WHERE (user_id = u_friend_id AND friend_id = u_user_id) or (user_id = u_user_id AND friend_id = u_friend_id);
	end if;
    END */$$
DELIMITER ;

/* Procedure structure for procedure `Register` */

/*!50003 DROP PROCEDURE IF EXISTS  `Register` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`localhost` PROCEDURE `Register`(
    IN user_phone VARCHAR(12),
    IN user_name VARCHAR(255),
    IN user_password VARCHAR(255),
    OUT new_user_id VARCHAR(20)  -- 修改为 VARCHAR 类型输出参数
)
BEGIN
    DECLARE temp_user_id VARCHAR(20);
    DECLARE new_user_email VARCHAR(30);
    
    -- 检查手机号是否已经存在
    IF EXISTS (SELECT 1 FROM m_user WHERE user_phone = m_user.user_phone) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = '该手机号已被注册！';
    END IF;
    
    -- 生成新的用户 ID（纯数字）
    #SET temp_user_id = FLOOR(100000000 + (RAND() * 900000000));  -- 生成 9 位数字
    SET temp_user_id = user_phone;
    -- 确保新生成的用户 ID 唯一
    #WHILE EXISTS(SELECT 1 FROM m_user WHERE user_id = temp_user_id) DO
    #    SET temp_user_id = FLOOR(100000000 + (RAND() * 900000000));  -- 再次生成新的数字
    #END WHILE;
    
    -- 生成新用户的邮箱
    SET new_user_email = CONCAT(temp_user_id , '@an.com');
    
    -- 插入新的用户记录
    INSERT INTO m_user(user_id, user_phone, user_name, user_password, user_email, user_created)
    VALUES (temp_user_id, user_phone, user_name, user_password, new_user_email, NOW());
    
    -- 设置输出参数 new_user_id 为生成的用户 ID
    SET new_user_id = temp_user_id;
END */$$
DELIMITER ;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
