#encoding: utf-8
require 'spec_helper'
require 'jailman/provisioner'

describe Jailman::Provisioner do
  let(:jail) {Jailman::Jail.new('my_jail')}

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

  describe '#create_rootfs' do
    let(:config) do
      jail = Jailman::Jail.new("Jail#{rand(1000)}")
      jail.directory = Dir.pwd + "/spec/fixtures/jails/#{jail.name}"
      provisioner = described_class.new(jail)
      provisioner
    end

    it 'creates a new rootfs' do
      provisioner = described_class.new(jail)
      provisioner.rootfs_script = "#{Dir.pwd}/bin/ps_rootfs"

      pending
      #provisioner.create_rootfs
      #expect(File.exists?(provisioner.jail.directory + "/data"))
      #expect(File.exists?(provisioner.jail.directory + "/rootfs"))
    end
  end

end
