#encoding: utf-8
require 'spec_helper'
require 'jailman/jail'

describe Jailman::Jail do

  context 'when initialized' do

    context 'without a name' do
      let(:jail) { described_class.new }

      it 'populates directory' do
        expect(jail.directory).to_not be_nil
        expect(jail.directory).to match(Dir.pwd)
      end

      it 'and populates name with the last element of directory' do
        expect(jail.name).to_not be_nil
        expect(jail.name).to match (jail.directory.split('/').last)
      end

    end

    context 'with a name' do
      let(:jail) { described_class.new('my_jail') }

      it 'populates directory' do
        expect(jail.directory).to_not be_nil
        expect(jail.directory).to match(Dir.pwd)
      end

      it 'and populates name with the constructor argument' do
        expect(jail.name).to match('my_jail')
      end

    end

  end

  describe '#create' do

    let(:jail) do
      jail = jail_factory
      jail.create
      jail
    end

    it 'changes its status to STARTED' do
      expect(jail.running?).to be_true
    end

    it 'runs a jail and be able to receive commands' do
      output = Jailman::CommandRunner.run("/usr/local/bin/psc #{jail.name} --run free -m")
      expect(output).to match("Mem:")
    end

    after { jail.stop }

  end


  describe '#run' do

    let(:jail) do
      jail = jail_factory
      jail.create
      jail
    end

    it 'runs a jail and be able to receive commands' do
      output = jail.run("echo testing")
      expect(output).to match("testing")
    end

   after { jail.stop }

  end

  describe '#stop' do
    let(:jail) do
      jail = jail_factory
      jail.create

      jail.stop
      jail
    end

    it 'changes status to STOPPED' do
      expect(jail.running?).to be_false
    end
  end

  describe '#status' do
    let(:jail) do
      jail = jail_factory
      jail.create
      jail
    end

    context 'when running' do
      it "returns correct status" do
        expect(jail.status).to match("STARTED")
      end
    end

    context 'when not running' do
      it "returns correct status" do
        expect(jail_factory.status).to match("STOPPED")
      end
    end

    after { jail.stop }
  end

end
