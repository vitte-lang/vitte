#!/usr/bin/env ruby

require "json"
require "open3"
require "shellwords"
require "yaml"

ROOT = File.expand_path("..", __dir__)

def fail_with(message)
  warn "[docs-examples] ERROR: #{message}"
  exit 1
end

version = File.read(File.join(ROOT, "VERSION")).strip
config = YAML.load_file(File.join(ROOT, "docs/_config.yml"))
fail_with("docs/_config.yml has vitte_version #{config["vitte_version"].inspect}, but VERSION is #{version}") unless config["vitte_version"] == version

["docs/exemples.html", "docs/reference/support.html"].each do |relative_path|
  path = File.join(ROOT, relative_path)
  source = File.read(path)
  match = source.match(/\A---\s*\n(.*?)\n---\s*\n/m)
  fail_with("#{relative_path} has no valid YAML front matter") unless match

  metadata = YAML.safe_load(match[1], aliases: false)
  reviewed_version = metadata["vitte_version"]
  next if reviewed_version == version

  fail_with("#{relative_path} records Vitte #{reviewed_version.inspect}; review it and set vitte_version to #{version}")
end

%w[vitte vitte-bootstrap].each do |binary|
  path = File.join(ROOT, "build/bin", binary)
  fail_with("missing built compiler #{path}; run make first") unless File.executable?(path)

  stdout, stderr, status = Open3.capture3(path, "--version", chdir: ROOT)
  reported = stdout.match(/^vitte\s+(\S+)/)&.captures&.first
  fail_with("#{binary} did not report version #{version}: #{stdout}#{stderr}") unless status.success? && reported == version
end

examples_path = File.join(ROOT, "docs/_data/examples.yml")
examples = YAML.load_file(examples_path)
fail_with("#{examples_path} must contain a non-empty list") unless examples.is_a?(Array) && !examples.empty?

titles = []
errors = []
known_modes = ["Bootstrap", "Frontend natif", "Compatibilité CLI", "Échec attendu"]

examples.each do |example|
  title = example["title"]
  titles << title
  required = %w[title group summary source lesson mode command result]
  missing = required.reject { |field| example[field].is_a?(String) && !example[field].empty? }
  errors << "#{title || "unnamed example"}: missing fields #{missing.join(", ")}" unless missing.empty?
  next unless missing.empty?

  unless known_modes.include?(example["mode"])
    errors << "#{title}: unknown mode #{example["mode"].inspect}"
    next
  end

  [example["source"], example["companion"]].compact.each do |source|
    errors << "#{title}: missing source file #{source}" unless File.file?(File.join(ROOT, source))
  end

  lesson = example["lesson"].delete_prefix("/").delete_suffix("/")
  errors << "#{title}: missing lesson page docs/#{lesson}.html" unless File.file?(File.join(ROOT, "docs", "#{lesson}.html"))

  tokens = Shellwords.split(example["command"])
  environment = {}
  while tokens.first&.match?(/\A[A-Za-z_][A-Za-z0-9_]*=/)
    key, value = tokens.shift.split("=", 2)
    environment[key] = value
  end
  if tokens.empty?
    errors << "#{title}: command is empty"
    next
  end

  begin
    stdout, stderr, status = Open3.capture3(environment, *tokens, chdir: ROOT)
  rescue SystemCallError => error
    errors << "#{title}: command could not start: #{error.message}"
    next
  end

  expected_exit = example["mode"] == "Échec attendu" ? 1 : 0
  documented_exit = example["result"].match(/code de sortie\s+(\d+)/i)&.captures&.first&.to_i
  if documented_exit != expected_exit
    errors << "#{title}: result must document expected exit code #{expected_exit}"
    next
  end

  if status.exitstatus != expected_exit
    errors << "#{title}: expected exit #{expected_exit}, got #{status.exitstatus || "signal #{status.termsig}"}; stdout=#{stdout.inspect}; stderr=#{stderr.inspect}"
    next
  end

  if expected_exit == 1
    expected_diagnostic = example["result"].match(/\b(?:VITTE_[A-Z0-9_]+|E\d{4})\b/)&.to_s
    actual_diagnostic = nil

    if expected_diagnostic&.start_with?("VITTE_")
      begin
        diagnostic_report = JSON.parse(stderr)
        actual_diagnostic = diagnostic_report.dig("diagnostics", 0, "internal_code")
      rescue JSON::ParserError
        actual_diagnostic = nil
      end
    elsif expected_diagnostic
      actual_diagnostic = expected_diagnostic if stderr.match?(/\b#{Regexp.escape(expected_diagnostic)}\b/)
    end

    unless expected_diagnostic && actual_diagnostic == expected_diagnostic
      errors << "#{title}: expected diagnostic #{expected_diagnostic.inspect}, got #{actual_diagnostic.inspect}; stderr=#{stderr.inspect}"
      next
    end
  end

  expected_output = example["result"].match(/«([^»]+)»/)&.captures&.first&.strip
  output_to_check = expected_exit == 1 ? stderr : stdout
  output_name = expected_exit == 1 ? "stderr" : "stdout"
  if expected_output && !output_to_check.include?(expected_output)
    errors << "#{title}: expected #{output_name} to include #{expected_output.inspect}, got #{output_to_check.inspect}"
    next
  end

  puts "[docs-examples] PASS #{title} (exit #{expected_exit})"
end

errors << "example titles must be unique" unless titles.compact.uniq.length == titles.compact.length
unless errors.empty?
  errors.each { |error| warn "[docs-examples] ERROR: #{error}" }
  exit 1
end

puts "[docs-examples] #{examples.length} examples and support review verified against Vitte #{version}."
