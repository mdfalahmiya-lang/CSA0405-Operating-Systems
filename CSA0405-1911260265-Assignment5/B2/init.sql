-- Sample result rows for the Assignment 5 portal.
CREATE TABLE IF NOT EXISTS results (
    register   CHAR(10) PRIMARY KEY,
    name       TEXT        NOT NULL,
    semester   SMALLINT    NOT NULL,
    gpa        NUMERIC(3,2) NOT NULL,
    status     TEXT        NOT NULL
);

INSERT INTO results (register, name, semester, gpa, status) VALUES
    ('1911260265', 'Mohammed Falah Miya', 5, 8.42, 'PASS'),
    ('1911260001', 'A. Student',          5, 7.81, 'PASS'),
    ('1911260002', 'B. Student',          5, 5.96, 'REVIEW')
ON CONFLICT (register) DO NOTHING;
