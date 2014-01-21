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


    private

    def create_file
      json = JSON.generate({
        :name   => jail.name,
        :rootfs => jail.directory
      })

      f = File.open(file_path, "w+")
      f.puts(json)
      f.close
    end

  end
end
