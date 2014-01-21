#encoding: utf-8
require 'json'

module Jailman
  class State

    attr_reader :jail, :file_path

    def initialize(jail=nil)
      raise ArgumentError.new("jail must be present") unless jail
      @jail = jail
      @file_path = "#{Jailman::Constants::STATE_DIR}/#{jail.name}"
    end

    def save
      raise "jail must be present" unless jail
      create_file
    end

    def load
      raise "jail must be present" unless jail
      return nil unless File.exists?(file_path)

      json = JSON.parse(File.read(file_path))
      jail = Jailman::Jail.new(json["name"], json["directory"])
    end

    def clear
      return unless File.exists?(file_path)
      FileUtils.rm_rf(file_path)
    end

    private

    def create_file
      json = JSON.generate({
        :pid       => jail.pid,
        :name      => jail.name,
        :directory => jail.directory
      })

      f = File.open(file_path, "w+")
      f.puts(json)
      f.close
    end

  end
end
