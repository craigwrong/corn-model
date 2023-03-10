import Foundation

public extension Tx {
    
    struct Out: Equatable {
        public init(network: Network = .main, value: UInt64, scriptPubKey: Script) {
            self.network = network
            self.value = value
            self.scriptPubKey = scriptPubKey
        }
        
        public var network: Network
        public var value: UInt64 // Sats
        public var scriptPubKey: Script
    }
}

extension Tx.Out {
    var memSize: Int {
        MemoryLayout.size(ofValue: value) + scriptPubKey.memSize
    }
}

public extension Tx.Out {

    var doubleValue: Double {
        Double(value) / 100_000_000
    }

    var data: Data {
        return valueData + scriptPubKey.data()
    }

    var valueData: Data {
        withUnsafeBytes(of: value) { Data($0) }
    }
    
    var address: String {
        if scriptPubKey.scriptType == .witnessV0KeyHash || scriptPubKey.scriptType == .witnessV0ScriptHash {
            return (try? SegwitAddrCoder(bech32m: false).encode(hrp: network.bech32HRP, version: 0, program: scriptPubKey.witnessProgram)) ?? ""
        } else if scriptPubKey.scriptType == .witnessV1TapRoot {
            return (try? SegwitAddrCoder(bech32m: true).encode(hrp: network.bech32HRP, version: 1, program: scriptPubKey.witnessProgram)) ?? ""
        }
        return ""
    }
    
    init(_ data: Data) {
        let value = data.withUnsafeBytes { $0.loadUnaligned(as: UInt64.self) }
        let data = data.dropFirst(MemoryLayout.size(ofValue: value))
        let scriptPubKey = Script(data)
        self.init(value: value, scriptPubKey: scriptPubKey)
    }
}
