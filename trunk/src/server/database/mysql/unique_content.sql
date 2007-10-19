DROP TABLE IF EXISTS unique_content;
CREATE TABLE `unique_content` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `content` text,
  PRIMARY KEY (`id`)
);

### Our book text for both writable and non-writable books

INSERT INTO unique_content VALUES (1, 'The Ancient Tome of Test:  Years ago, some guy was lame' );
INSERT INTO unique_content VALUES (2, 'Bards tell that there are magical ways to fix bugs. The knowledge you have to obtain can be found in the Dark Way.' );



