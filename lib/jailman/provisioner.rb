#encoding: utf-8

module Jailman
  class Provisioner

    attr_reader :jail

    def initialize(jail=nil)
      raise ArgumentError.new('A jail must be passed') unless jail
      @jail = jail
    end

    def run!
      create_jail
      create_rootfs
    end

    def status
      #TODO: NOw
    end

    private

    def create_jail
      command = "#{Jailman::Constants::JAIL_SCRIPT} #{jail.name} --create"
      pid = Jailman::CommandRunner.run(command)
      create_pid_file(pid)
    end

    def create_pid_file(pid)
      command = "echo #{pid.chomp} > #{Jailman::Constants::PID_DIR}/#{jail.name}.pid"
      Jailman::CommandRunner.run(command)
    end

    def create_rootfs
      command = "#{Jailman::Constants::ROOTFS_SCRIPT} #{jail.name} #{jail.directory}"
      Jailman::CommandRunner.run(command)
    end

  end
end
