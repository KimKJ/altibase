DROP PROCEDURE init_table;

CREATE OR REPLACE PROCEDURE init_table ()
AS
BEGIN
    FOR i IN 1 .. 10000 LOOP
    INSERT INTO t0 VALUES (i);
    INSERT INTO t1 VALUES (i);
    INSERT INTO t2 VALUES (i);
    INSERT INTO t3 VALUES (i);
    INSERT INTO t4 VALUES (i);
    INSERT INTO t5 VALUES (i);
    INSERT INTO t6 VALUES (i);
    INSERT INTO t7 VALUES (i);
    INSERT INTO t8 VALUES (i);
    INSERT INTO t9 VALUES (i);
    END LOOP;
END;
/

EXEC INIT_TABLE();
