INSERT INTO users VALUES (1, 'Alice', 30);
INSERT INTO users VALUES (2, 'Bob', 24);
INSERT INTO users VALUES (3, 'Charlie', 29);
SELECT * FROM users;
SELECT id, name FROM users WHERE age >= 29;
INSERT INTO books VALUES (10, 'CSAPP', 'Bryant', 42000);
INSERT INTO books VALUES (11, 'Clean Code', 'Martin', 33000);
SELECT title, author FROM books WHERE price >= 40000;
