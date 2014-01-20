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
      command = "#{jail_script} #{jail.name} --create"
      Jailman::CommandRunner.run(command)
    end

    def create_rootfs
      command = "#{rootfs_script} #{jail.name} #{jail.directory}"
      Jailman::CommandRunner.run(command)
    end

  end
end
