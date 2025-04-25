0 3 * * * mysql sensores -e "DELETE FROM datos WHERE timestamp < NOW() - INTERVAL 30 DAY;"
