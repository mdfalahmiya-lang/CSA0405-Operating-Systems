# Container image for the campus result-portal web service (Flask + gunicorn).
FROM python:3.12-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# TODO 1 (completed): set the working directory inside the image to /app.
WORKDIR /app

# Install the dependencies first so this layer is cached between builds.
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# TODO 2 (completed): copy the application source into the image and run it
# as a non-root user named portal.
COPY app.py init.sql ./
RUN useradd --create-home --uid 10001 portal \
    && chown -R portal:portal /app
USER portal

# TODO 3 (completed): document port 8000 and start gunicorn on 0.0.0.0:8000
# using the app object in app.py.
EXPOSE 8000
CMD ["gunicorn", "--bind", "0.0.0.0:8000", "--workers", "2", \
     "--threads", "4", "--access-logfile", "-", "app:app"]
