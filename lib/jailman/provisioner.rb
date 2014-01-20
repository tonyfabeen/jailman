#encoding: utf-8

module Jailman
  class Provisioner

    attr_reader :jail
    attr_reader :rootfs_script
    attr_reader :jail_script

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
      @rootfs_script = Jailman::Constants::ROOTFS_SCRIPT
      @jail_script   = Jailman::Constants::JAIL_SCRIPT
    end

    def run!
      create_jail
      create_rootfs
    end

    private

    def create_jail
      script = "#{jail_script} #{jail.name} --create"
      output = `#{script}`
      raise output unless $? == 0
    end

    def create_rootfs
      script = "#{rootfs_script} #{jail.name} #{jail.directory}"
      output = `#{script}`
      raise output unless $? == 0
    end

  end
end
