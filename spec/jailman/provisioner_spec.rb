#encoding: utf-8
require 'spec_helper'
require 'jailman/provisioner'
require 'jailman/constants'
require 'jailman/command_runner'

describe Jailman::Provisioner do
  let(:jail) { jail_factory }

  context 'when initialized' do
    context 'and no jail passed as argument' do

      it 'raises an Exception' do
        expect{ described_class.new }.to raise_error('A jail must be passed')
      end

    end

    context 'when a jail is filled' do

      it 'populates jail' do
        config  = described_class.new(jail)

        expect(config.jail).to_not be_nil
        expect(config.jail).to be(jail)
      end

    end
  end

  describe '#run!' do
    let(:provisioner) do
      provisioner = described_class.new(jail)
      provisioner.run!
      provisioner
    end

    it 'creates a new rootfs' do
      output = Jailman::CommandRunner.run("/usr/local/bin/psc #{provisioner.jail.name} --run free -m")
      expect(output).to match("Mem:")
    end

    it 'creates a pid file' do
      pid_file = Jailman::Constants::PID_DIR + "/#{provisioner.jail.name}.pid"
      expect(File.exists?(pid_file)).to be_true
    end

   after do
     Jailman::CommandRunner.run("/usr/local/bin/psc #{provisioner.jail.name} --kill")

   end

  end

end
