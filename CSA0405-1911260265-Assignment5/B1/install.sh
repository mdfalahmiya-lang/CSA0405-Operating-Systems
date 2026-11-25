#!/usr/bin/env bash
# Install the B1 result portal as a systemd service behind Nginx, with
# firewall rules and cron jobs. Run as root from the repository.
set -euo pipefail

B1_DIR=$(cd "$(dirname "$0")" && pwd)
PORTAL_HOME=/opt/result-portal
LOG_DIR=/var/log/result-portal

if ! id portal >/dev/null 2>&1; then
    useradd --create-home --home-dir "$PORTAL_HOME" \
        --shell /usr/sbin/nologin portal
fi

install -d -o portal -g portal "$PORTAL_HOME"
install -d -o portal -g adm -m 2750 "$LOG_DIR"
install -m 644 "$B1_DIR/portal/app.py" "$B1_DIR/portal/requirements.txt" \
    "$B1_DIR/portal/init.sql" "$PORTAL_HOME/"
install -d -o portal -g portal "$PORTAL_HOME/scripts"
install -m 755 "$B1_DIR"/scripts/*.sh "$PORTAL_HOME/scripts/"

if [ ! -x "$PORTAL_HOME/venv/bin/python" ]; then
    runuser -u portal -- python3 -m venv "$PORTAL_HOME/venv"
fi
runuser -u portal -- "$PORTAL_HOME/venv/bin/pip" install --quiet --upgrade pip
runuser -u portal -- "$PORTAL_HOME/venv/bin/pip" install --quiet \
    -r "$PORTAL_HOME/requirements.txt"
chown -R portal:portal "$PORTAL_HOME"

install -m 644 "$B1_DIR/systemd/result-portal.service" \
    /etc/systemd/system/result-portal.service
systemctl daemon-reload
systemctl enable --now result-portal

install -m 644 "$B1_DIR/nginx/result-portal.conf" \
    /etc/nginx/sites-available/result-portal
ln -sf /etc/nginx/sites-available/result-portal \
    /etc/nginx/sites-enabled/result-portal
rm -f /etc/nginx/sites-enabled/default
nginx -t
systemctl enable --now nginx
systemctl reload nginx

install -m 644 "$B1_DIR/logrotate/result-portal" \
    /etc/logrotate.d/result-portal
crontab "$B1_DIR/cron/result-portal.cron"
systemctl enable --now cron

# B1.3: only SSH, HTTP and HTTPS are allowed in.
ufw --force reset >/dev/null
ufw default deny incoming >/dev/null
ufw default allow outgoing >/dev/null
ufw allow 22/tcp >/dev/null
ufw allow 80/tcp >/dev/null
ufw allow 443/tcp >/dev/null
ufw --force enable >/dev/null

echo "installed: $(systemctl is-active result-portal) portal, $(systemctl is-active nginx) nginx, ufw $(ufw status | head -1 | awk '{print $2}')"
