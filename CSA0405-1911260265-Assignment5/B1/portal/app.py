import os
import socket
import time
from datetime import datetime, timezone

from flask import Flask, jsonify

app = Flask(__name__)
START = time.time()
HOST = socket.gethostname()


def query_result(register):
    """Return a row from PostgreSQL when DATABASE_URL is configured."""
    url = os.environ.get("DATABASE_URL")
    if not url:
        return None
    import psycopg

    with psycopg.connect(url, connect_timeout=2) as conn:
        with conn.cursor() as cur:
            cur.execute(
                "SELECT register, name, semester, gpa, status "
                "FROM results WHERE register = %s",
                (register,),
            )
            return cur.fetchone()


@app.get("/")
def index():
    return (
        "<!doctype html><html><head><title>SIMATS Result Portal</title></head>"
        "<body style='font-family:Arial;margin:40px'>"
        "<h1>SIMATS Result Portal</h1>"
        "<p>Semester results are published here on result day.</p>"
        f"<p>Instance: <b>{HOST}</b> &middot; PID {os.getpid()} &middot; "
        f"up {time.time() - START:.0f} s</p>"
        "<p>Try <code>/health</code> or <code>/api/results/1911260265</code>.</p>"
        "</body></html>"
    )


@app.get("/health")
def health():
    return jsonify(
        status="ok",
        host=HOST,
        pid=os.getpid(),
        uptime_s=round(time.time() - START, 1),
        time=datetime.now(timezone.utc).isoformat(),
    )


@app.get("/ping")
def ping():
    # Constant-length response used by the load generator so that ApacheBench
    # does not flag normal JSON length changes as failed requests.
    return "ok\n"


@app.get("/api/results/<register>")
def result(register):
    if not (register.isdigit() and len(register) == 10):
        return jsonify(error="register number must be 10 digits"), 400

    try:
        row = query_result(register)
    except Exception as exc:  # database unavailable or not initialised
        row = None
        db_error = type(exc).__name__
    else:
        db_error = None

    if row:
        return jsonify(
            register=row[0], name=row[1], semester=row[2],
            gpa=float(row[3]), status=row[4], host=HOST, source="postgres",
        )

    # Deterministic fallback so the service still answers if the database is down.
    seed = sum(int(c) for c in register)
    return jsonify(
        register=register, name="Student", semester=5,
        gpa=round(6.5 + (seed % 30) / 10, 1),
        status="PASS" if seed % 7 else "REVIEW",
        host=HOST, source="fallback", db_error=db_error,
    )


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", "8000")))
