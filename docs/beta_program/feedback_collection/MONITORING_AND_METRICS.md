# Monitoring and Metrics

## What to monitor
- Crash reports (count, reproducibility, severity)
- Performance complaints (latency/throughput pain points)
- API friction (confusing names, missing abstractions)

## Quantitative metrics captured weekly
- `loc_written`
- `time_to_productivity_hours`
- `pain_points_count`
- `critical_bugs_count`
- `core_api_sentiment_positive` (boolean per tester/week)
- `real_world_project_progress` (none/in-progress/completed)

## Data collection format
- CSV per week under `data/beta_feedback/week-<n>.csv`
- One row per tester

## Rollup KPIs (acceptance)
- >= 80% positive core API sentiment
- < 2 critical bugs per tester
- Average learning curve < 4 hours
- >= 1 completed real-world project
